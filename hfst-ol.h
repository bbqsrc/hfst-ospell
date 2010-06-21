/* -*- Mode: C++ -*- */

/*
 * This file contains some classes, typedefs and constant common to all
 * hfst-optimized-lookup stuff. This is just to get them out of the way
 * in the actual ospell code.
 */

#include <vector>
#include <map>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <set>

#ifndef HFST_OSPELL_HFST_OL_H_
#define HFST_OSPELL_HFST_OL_H_

typedef unsigned short SymbolNumber;
typedef unsigned int TransitionTableIndex;
typedef std::vector<SymbolNumber> SymbolVector;
typedef std::vector<std::string> KeyTable;
typedef std::map<std::string, SymbolNumber> StringSymbolMap;
typedef short ValueNumber;

// Forward declarations to typedef some more containers
class TransitionIndex;
class Transition;
class FlagDiacriticOperation;

typedef std::vector<TransitionIndex*> TransitionIndexVector;
typedef std::vector<Transition*> TransitionVector;
typedef std::vector<FlagDiacriticOperation> OperationVector;

const SymbolNumber NO_SYMBOL_NUMBER = USHRT_MAX;
const TransitionTableIndex NO_TABLE_INDEX = UINT_MAX;

// This is 2^31, hopefully equal to UINT_MAX/2 rounded up.
// For some profound reason it can't be replaced with (UINT_MAX+1)/2.
const TransitionTableIndex TRANSITION_TARGET_TABLE_START = 2147483648u;

// the flag diacritic operators as given in
// Beesley & Karttunen, Finite State Morphology (U of C Press 2003)
enum FlagDiacriticOperator {P, N, R, D, C, U};

enum HeaderFlag {Weighted, Deterministic, Input_deterministic, Minimized,
		 Cyclic, Has_epsilon_epsilon_transitions,
		 Has_input_epsilon_transitions, Has_input_epsilon_cycles,
		 Has_unweighted_input_epsilon_cycles};

class TransducerHeader
{
 private:
  SymbolNumber number_of_symbols;
  SymbolNumber number_of_input_symbols;
  TransitionTableIndex size_of_transition_index_table;
  TransitionTableIndex size_of_transition_target_table;

  TransitionTableIndex number_of_states;
  TransitionTableIndex number_of_transitions;

  bool weighted;
  bool deterministic;
  bool input_deterministic;
  bool minimized;
  bool cyclic;
  bool has_epsilon_epsilon_transitions;
  bool has_input_epsilon_transitions;
  bool has_input_epsilon_cycles;
  bool has_unweighted_input_epsilon_cycles;

  void read_property(bool &property, FILE * f)
  {
    unsigned int prop;
    unsigned int ret;
    ret = fread(&prop,sizeof(unsigned int),1,f);
    if (prop == 0)
      {
	property = false;
	return;
      }
    else
      {
	property = true;
	return;
      }
    std::cerr << "Could not parse transducer; wrong or corrupt file?" << std::endl;
    exit(1);
  }

 public:
  TransducerHeader(FILE * f)
    {
      // The silly compiler complains about not catching the return value
      // of fread(). Hence this dummy variable is needed.
      size_t val;

      val = fread(&number_of_input_symbols,sizeof(SymbolNumber),1,f);
      val = fread(&number_of_symbols,sizeof(SymbolNumber),1,f);

      val = fread(&size_of_transition_index_table,sizeof(TransitionTableIndex),1,f);
      val = fread(&size_of_transition_target_table,sizeof(TransitionTableIndex),1,f);

      val = fread(&number_of_states,sizeof(TransitionTableIndex),1,f);
      val = fread(&number_of_transitions,sizeof(TransitionTableIndex),1,f);

      read_property(weighted,f);

      read_property(deterministic,f);
      read_property(input_deterministic,f);
      read_property(minimized,f);
      read_property(cyclic,f);
      read_property(has_epsilon_epsilon_transitions,f);
      read_property(has_input_epsilon_transitions,f);
      read_property(has_input_epsilon_cycles,f);
      read_property(has_unweighted_input_epsilon_cycles,f);

      // For ospell: demand weightedness
/*      if (!weighted) {
	  std::cerr << "Transducers must be weighted!\n" << std::endl;
	  exit(1);
	  }*/
    }

  SymbolNumber symbol_count(void)
  { return number_of_symbols; }

  SymbolNumber input_symbol_count(void)
  { return number_of_input_symbols; }
  
  TransitionTableIndex index_table_size(void)
  { return size_of_transition_index_table; }

  TransitionTableIndex target_table_size(void)
  { return size_of_transition_target_table; }

  bool probe_flag(HeaderFlag flag)
  {
    switch (flag) {
    case Weighted: return weighted;
    case Deterministic: return deterministic;
    case Input_deterministic: return input_deterministic;
    case Minimized: return minimized;
    case Cyclic: return cyclic;
    case Has_epsilon_epsilon_transitions: return has_epsilon_epsilon_transitions;
    case Has_input_epsilon_transitions: return has_input_epsilon_transitions;
    case Has_input_epsilon_cycles: return has_input_epsilon_cycles;
    case Has_unweighted_input_epsilon_cycles: return has_unweighted_input_epsilon_cycles;
    }
    return false;
  }
};

class FlagDiacriticOperation
{
 private:
  FlagDiacriticOperator operation;
  SymbolNumber feature;
  ValueNumber value;
 public:
 FlagDiacriticOperation(FlagDiacriticOperator op, SymbolNumber feat, ValueNumber val):
  operation(op), feature(feat), value(val) {}

  // dummy constructor
 FlagDiacriticOperation():
  operation(P), feature(NO_SYMBOL_NUMBER), value(0) {}
  
  bool isFlag(void) { return feature != NO_SYMBOL_NUMBER; }
  FlagDiacriticOperator Operation(void) { return operation; }
  SymbolNumber Feature(void) { return feature; }
  ValueNumber Value(void) { return value; }

};

class TransducerAlphabet
{
private:
    SymbolNumber number_of_symbols;
    KeyTable * kt;
    OperationVector operations;
    SymbolNumber other_symbol;
    std::set<SymbolNumber> flags;
    StringSymbolMap string_to_symbol;

  void get_next_symbol(FILE * f, SymbolNumber k);

  char * line;

  std::map<std::string, SymbolNumber> feature_bucket;
  std::map<std::string, ValueNumber> value_bucket;
  ValueNumber val_num;
  SymbolNumber feat_num;
 
public:
    TransducerAlphabet(FILE * f, SymbolNumber symbol_number):
	number_of_symbols(symbol_number),
	kt(new KeyTable),
	operations(),
	other_symbol(NO_SYMBOL_NUMBER),
	string_to_symbol(StringSymbolMap()),
	line((char*)(malloc(1000)))
	{
	    feat_num = 0;
	    val_num = 1;
	    value_bucket[std::string()] = 0; // empty value = neutral
	    // assume the first symbol is epsilon which we don't want to print
	    kt->push_back(std::string(""));
	    for (SymbolNumber k = 0; k < number_of_symbols; ++k)
	    {
		get_next_symbol(f,k);
	    }
	    free(line);
	}
    
    KeyTable * get_key_table(void)
	{ return kt; }
    
    OperationVector get_operation_vector(void)
	{ return operations; }
    
    SymbolNumber get_state_size(void)
	{ return feature_bucket.size(); }
    
    SymbolNumber get_other(void)
	{
	    return other_symbol;
	}

    void free_temporary(void)
	{
	    string_to_symbol.clear();
	    feature_bucket.clear();
	    value_bucket.clear();
	    flags.clear();
	}

    StringSymbolMap * get_string_to_symbol(void)
	{
	    return &string_to_symbol;
	}

    bool is_flag(SymbolNumber symbol)
	{
	    return flags.count(symbol) == 1;
	}
  
};

class LetterTrie;
typedef std::vector<LetterTrie*> LetterTrieVector;

class LetterTrie
{
 private:
  LetterTrieVector letters;
  SymbolVector symbols;

 public:
 LetterTrie(void):
  letters(UCHAR_MAX, static_cast<LetterTrie*>(NULL)),
    symbols(UCHAR_MAX,NO_SYMBOL_NUMBER)
      {}

  void add_string(const char * p,SymbolNumber symbol_key);

  SymbolNumber find_key(char ** p);

};

class Encoder {

 private:
  SymbolNumber number_of_input_symbols;
  LetterTrie letters;
  SymbolVector ascii_symbols;

  void read_input_symbols(KeyTable * kt);

 public:
 Encoder(KeyTable * kt, SymbolNumber input_symbol_count):
  number_of_input_symbols(input_symbol_count),
    ascii_symbols(UCHAR_MAX,NO_SYMBOL_NUMBER)
      {
	read_input_symbols(kt);
      }
  
  SymbolNumber find_key(char ** p);
};

typedef std::vector<ValueNumber> FlagDiacriticState;

class TransitionIndex
{
 protected:
  SymbolNumber input_symbol;
  TransitionTableIndex first_transition_index;
  
 public:
  
  // Each TransitionIndex has an input symbol and a target index.
  static const size_t SIZE = 
    sizeof(SymbolNumber) + sizeof(TransitionTableIndex);

 TransitionIndex(SymbolNumber input,
		 TransitionTableIndex first_transition):
    input_symbol(input),
    first_transition_index(first_transition)
    {}

  bool matches(SymbolNumber s);
  
  TransitionTableIndex target(void)
  {
    return first_transition_index;
  }
  
  bool final(void)
  {
    return first_transition_index == 1;
  }
  
  SymbolNumber get_input(void)
  {
    return input_symbol;
  }
};

class Transition
{
 protected:
  SymbolNumber input_symbol;
  SymbolNumber output_symbol;

  TransitionTableIndex target_index;

 public:

  // Each transition has an input symbol an output symbol and 
  // a target index.
  static const size_t SIZE = 
    2 * sizeof(SymbolNumber) + sizeof(TransitionTableIndex);

 Transition(SymbolNumber input,
	    SymbolNumber output,
	    TransitionTableIndex target):
    input_symbol(input),
    output_symbol(output),
    target_index(target)
    {}

  bool matches(SymbolNumber s);

  TransitionTableIndex target(void)
  {
    return target_index;
  }

  SymbolNumber get_output(void)
  {
    return output_symbol;
  }

  SymbolNumber get_input(void)
  {
    return input_symbol;
  }
  
  bool final(void)
  {
    return target_index == 1;
  }
};

class IndexTableReader
{
 private:
  TransitionTableIndex number_of_table_entries;
  char * TableIndices;
  TransitionIndexVector indices;
  size_t table_size;
  
  void get_index_vector(void);
 public:
 IndexTableReader(FILE * f,
			 TransitionTableIndex index_count): 
  number_of_table_entries(index_count)
    {
      table_size = number_of_table_entries*TransitionIndex::SIZE;
      TableIndices = (char*)(malloc(table_size));

      // This dummy variable is needed, since the compiler complains
      // for not catching the return value of fread().
      int dummy_number_of_bytes;

      dummy_number_of_bytes = fread(TableIndices,table_size,1,f);
      get_index_vector();
    }
  
  bool get_finality(TransitionTableIndex i)
  {
    return indices[i]->final();
  }
  
  TransitionIndex * at(TransitionTableIndex i)
  {
    return indices[i];
  }
  
  TransitionIndexVector &operator() (void)
    { return indices; }
};

class TransitionTableReader
{
 protected:
  TransitionTableIndex number_of_table_entries;
  char * TableTransitions;
  TransitionVector transitions;
  size_t table_size;
  size_t transition_size;
  
  TransitionTableIndex position;
  
  void get_transition_vector(void);
 public:
 TransitionTableReader(FILE * f,
			      TransitionTableIndex transition_count):
  number_of_table_entries(transition_count),
    position(0)
      {
	table_size = number_of_table_entries*Transition::SIZE;
	TableTransitions = (char*)(malloc(table_size));
	int bytes;
	bytes = fread(TableTransitions,table_size,1,f);
	get_transition_vector();

      }
  
  void Set(TransitionTableIndex pos);

  Transition * at(TransitionTableIndex i)
  {
    return transitions[i - TRANSITION_TARGET_TABLE_START];
  }

  void Next(void)
  {
    ++position;
  }
  
  bool Matches(SymbolNumber s);

  TransitionTableIndex get_target(void)
  {
    return transitions[position]->target();
  }

  SymbolNumber get_output(void)
  {
    return transitions[position]->get_output();
  }

  SymbolNumber get_input(void)
  {
    return transitions[position]->get_input();
  }

  bool get_finality(TransitionTableIndex i);

  TransitionVector &operator() (void)
    { 
      return transitions; 
    }
};

#endif // HFST_OSPELL_HFST_OL_H_