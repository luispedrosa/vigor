#include "dir-24-8-basic.h"
//@ #include <nat.gh>
//@ #include <bitops.gh>

/*@
lemma void set_flag_entry_assumptions(uint16_t entry);
  requires true == is_entry24_valid(entry) &*& false == extract_flag(entry);
  ensures 0 <= entry &*& entry <= 0xFF;

lemma Z Z_of_uint8_custom(int x)
    requires 0 <= x &*& x <= 255;
    ensures result == Z_of_int(x, N8) &*& x == int_of_Z(result);
{
    return Z_of_uintN(x, N8);
}

lemma Z Z_of_uint16_custom(int x)
    requires 0 <= x &*& x <= 65535;
    ensures result == Z_of_int(x, N16) &*& x == int_of_Z(result);
{
    return Z_of_uintN(x, N16);
}

lemma Z Z_of_uint32_custom(int x)
    requires 0 <= x &*& x <= 0xffffffff;
    ensures result == Z_of_int(x, N32) &*& x == int_of_Z(result);
{
    return Z_of_uintN(x, N32);
}

lemma void flag_mask_or_x_begins_with_one(uint16_t x);
  requires true;
  ensures extract_flag(x | TBL_24_FLAG_MASK) == true;
  
lemma void flag_mask_or_x_not_affect_15LSB(uint16_t x);
  requires 0 <= x &*& x <= 0x7FFF;
  ensures x == ((x | TBL_24_FLAG_MASK) & 0x7FFF);

lemma void new_invalid(uint16_t *t, uint32_t i, uint32_t size);
  requires t[0..i] |-> ?l1 &*& l1 == repeat_n(nat_of_int(i), INVALID) &*& t[i..size] |-> ?l2 &*& l2 == cons(?v, ?cs0);
  ensures t[0..i+1] |-> ?l3 &*& l3 == append(l1, cons(INVALID, nil)) &*& l3 == repeat_n(nat_of_int(i+1), INVALID) &*& t[i+1..size] |-> cs0;

lemma void entries_24_mapping_invalid(fixpoint (uint16_t, option<pair<bool, Z> >) map_func, list<uint16_t> lst);
  requires length(lst) == TBL_24_MAX_ENTRIES;
  ensures map(map_func, lst) == repeat_n(nat_of_int(length(lst)), none);
  
lemma void entries_long_mapping_invalid(fixpoint (uint16_t, option<Z>) map_func, list<uint16_t> lst);
  requires length(lst) == TBL_LONG_MAX_ENTRIES;
  ensures map(map_func, lst) == repeat_n(nat_of_int(length(lst)), none);
  
lemma void new_value(uint16_t *t, uint32_t i, uint32_t size, uint16_t value);
  requires t[0..i] |-> ?l1 &*& t[i..size] |-> cons(?v, ?cs0);
  ensures t[0..i+1] |-> append(l1, cons(value, nil)) &*& t[i+1..size] |-> cs0;
  
lemma void no_update(uint16_t *t, uint32_t i, uint32_t size);
  requires t[0..i] |-> ?l1 &*& t[i..size] |-> cons(?v, ?cs0);
  ensures t[0..i+1] |-> append(l1, cons(v, nil)) &*& t[i+1..size] |-> cs0;
  
lemma void mapping_holds<t>(fixpoint (uint16_t, t) map_func, list<uint16_t> toMap, list<t> lst);
  requires length(toMap) == length(lst);
  ensures lst == map(map_func, toMap);
  
lemma void lookup_at_index<t>(uint32_t index, list<uint16_t> lst, list<t> mapped, fixpoint(uint16_t, t) map_func);
  requires 0 <= index &*& index < length(lst) &*& length(lst) == length(mapped);
  ensures map_func(nth(index, lst)) == nth(index, mapped);
  
lemma void invalid_is_none24(uint16_t entry, option<pair<bool, Z> > mapped);
  requires entry == INVALID &*& entry_24_mapping(entry) == mapped;
  ensures mapped == none;
  
lemma void valid_next_hop24(uint16_t entry, option<pair<bool, Z> > mapped);
  requires entry != INVALID &*& 0 <= entry &*& entry <= 0x7FFF &*& entry_24_mapping(entry) == mapped &*& mapped == some(?p) &*& p == pair(?b, ?v);
  ensures b == false &*& entry == int_of_Z(v);
  
lemma void valid_next_bucket_long(uint16_t entry, option<pair<bool, Z> > mapped);
  requires entry != INVALID &*& true == extract_flag(entry) &*& true == valid_entry24(entry) &*& entry_24_mapping(entry) == mapped &*& mapped == some(?p) &*& p == pair(?b, ?v);
  ensures b == true &*& extract_value(entry) == int_of_Z(v);

lemma void invalid_is_none_long(uint16_t entry, option<Z> mapped);
  requires entry == INVALID &*& entry_long_mapping(entry) == mapped;
  ensures mapped == none;
  
lemma void valid_next_hop_long(uint16_t entry, option<Z> mapped);
  requires entry != INVALID &*& 0 <= entry &*& entry <= 0x7FFF &*& entry_long_mapping(entry) == mapped &*& mapped == some(?v);
  ensures entry == int_of_Z(v);
  
lemma void enforce_map_invalid_is_valid(list<uint16_t> entries, fixpoint(uint16_t, bool) validation_func);
  requires entries == repeat_n(nat_of_int(length(entries)), INVALID) &*& true == validation_func(INVALID);
  ensures true == forall(entries, validation_func);

//If true == forall, then any elem is true  
lemma void elem_is_valid(list<uint16_t> entries, fixpoint(uint16_t, bool) validation_func, uint32_t index);
  requires true == forall(entries, validation_func) &*& 0 <= index &*& index < length(entries);
  ensures true == validation_func(nth(index, entries));
@*/

void fill_invalid(uint16_t *t, uint32_t size)
//@ requires t[0..size] |-> _;
//@ ensures t[0.. size] |-> repeat_n(nat_of_int(size), INVALID);
{ 
  for(uint32_t i = 0; i < size; i++)
  /*@ invariant 0 <= i &*& i <= size &*&
                t[0..i] |-> repeat_n(nat_of_int(i), INVALID) &*&
                t[i..size] |-> _;
  @*/
  {
    t[i] = INVALID;
    //@ new_invalid(t, i, size);
  }
}

uint32_t build_mask_from_prefixlen(uint8_t prefixlen)
//@ requires prefixlen <= 32;
//@ ensures result == int_of_Z(mask32_from_prefixlen(prefixlen));
{
  uint32_t ip_masks[33] = { 0x00000000, 0x80000000, 0xC0000000, 0xE0000000, 0xF0000000, 0xF8000000, 0xFC000000, 0xFE000000, 
                            0xFF000000, 0xFF800000, 0xFFC00000, 0xFFE00000, 0xFFF00000, 0xFFF80000, 0xFFFC0000, 0xFFFE0000,
                            0xFFFF0000, 0xFFFF8000, 0xFFFFC000, 0xFFFFE000, 0xFFFFF000, 0xFFFFF800, 0xFFFFFC00, 0xFFFFFE00,
                            0xFFFFFF00, 0xFFFFFF80, 0xFFFFFFC0, 0xFFFFFFE0, 0xFFFFFFF0, 0xFFFFFFF8, 0xFFFFFFFC, 0xFFFFFFFE, 0xFFFFFFFF};

  return ip_masks[prefixlen];
}

/**
 * Extract the 24 MSB of an uint8_t array and returns them
 */
uint32_t tbl_24_extract_first_index(uint32_t data)
//@ requires true;
/*@ ensures 0 <= result &*& result < pow_nat(2, nat_of_int(24)) &*&
            result == index24_from_ipv4(Z_of_int(data, N32));

@*/
{
  //@ Z d = Z_of_uint32_custom(data);
  //@ shiftright_def(data, d, N8);
  //@ shiftright_limits(data, N32, N8);
  uint32_t res = data >> BYTE_SIZE;
  //@ assert res == int_of_Z(Z_shiftright(d, N8));
  return res;
}


/**
 * Computes how many entries the rule will take
 */
uint32_t compute_rule_size(uint8_t prefixlen)
//@ requires prefixlen <= 32;
//@ ensures result == compute_rule_size(prefixlen) &*& prefixlen < 24 ? result <= pow_nat(2, nat_of_int(24)) : result <= pow_nat(2, N8);
{	
  if(prefixlen < 24){
    uint32_t res[24] = { 0x1000000, 0x800000, 0x400000, 0x200000, 0x100000, 0x80000, 0x40000, 0x20000, 0x10000,
                         0x8000, 0x4000, 0x2000, 0x1000, 0x800, 0x400, 0x200, 0x100 ,0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2};
    uint32_t v = res[prefixlen];
    return v;
  }else{
    uint32_t res[9] = {0x100 ,0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1};
    uint32_t v = res[prefixlen-24];
    return v;
  }
}

bool tbl_24_entry_flag(uint16_t entry)
//@ requires entry != INVALID &*& true == valid_entry24(entry) &*& entry_24_mapping(entry) == some(?p) &*& p == pair(?b, _);
//@ ensures result == extract_flag(entry) &*& result == b;
{
  return (entry >> 15) == 1;
}



uint16_t tbl_24_entry_set_flag(uint16_t entry)
/*@ requires entry != INVALID &*& 
             false == extract_flag(entry) &*& //entry should not point to tbl_long already
             true == valid_entry24(entry) &*& 
             fst(get_someOption24(entry_24_mapping(entry))) == false;
@*/
/*@ ensures result == set_flag(entry) &*& 
            true == extract_flag(result) &*&
            fst(get_someOption24(entry_24_mapping(result))) == true &*&
            snd(get_someOption24(entry_24_mapping(result))) == Z_of_int(entry, N16);
@*/
{
  //@ set_flag_entry_assumptions(entry);
  //@ bitor_limits(entry, TBL_24_FLAG_MASK, N16);
  uint16_t res = (uint16_t)(entry | TBL_24_FLAG_MASK);
  //@ Z mask = Z_of_uint16_custom(TBL_24_FLAG_MASK);
  //@ Z val = Z_of_uint16_custom(entry);
  //@ bitor_def(entry, val, TBL_24_FLAG_MASK, mask);
  
  //Prove that masking with 0x8000 begins with a one
  //@ flag_mask_or_x_begins_with_one(entry);
  
  //Prove that masking with 0x8000 does not affect the 15 LSB
  //@ flag_mask_or_x_not_affect_15LSB(entry);

  return res;
}


uint16_t tbl_long_extract_first_index(uint32_t data, uint8_t prefixlen, uint8_t base_index)
//@ requires 0 <= base_index &*& base_index < TBL_LONG_OFFSET_MAX &*& 0 <= prefixlen &*& prefixlen <= 32;
//@ ensures result == compute_starting_index_long(init_rule(data, prefixlen, 0), base_index) &*& 0 <= result &*& result <= 0xFFFF;//dummy route, unused
{   
  //@ lpm_rule rule = init_rule(data, prefixlen, 0); //any route is OK
  
  uint32_t mask = build_mask_from_prefixlen(prefixlen);
  //@ Z maskZ = Z_of_uint32_custom(mask);
  //@ Z d = Z_of_uint32_custom(data);
  //@ bitand_def(data, d, mask, maskZ);
  uint32_t masked_data = data & mask;
  //@ assert int_of_Z(Z_and(d, mask32_from_prefixlen(prefixlen))) == masked_data;
  
  //@ bitand_limits(data, 0xFF, N32);
  //@ Z masked_dataZ = Z_of_uint32_custom(masked_data);
  //@ Z byte_mask = Z_of_uint8_custom(0xFF);
  //@ bitand_def(masked_data, masked_dataZ, 0xFF, byte_mask);
  uint8_t last_byte = (uint8_t)(masked_data & 0xFF);
  //@ assert (masked_data & 0xFF) == last_byte;
  
  uint16_t res = (uint16_t)(base_index * (uint16_t)TBL_LONG_FACTOR + last_byte);
    
  return res;
}

struct tbl* tbl_allocate()
//@ requires true;
/*@ ensures result == 0 ? 
      true 
    : 
      table(result, 0, dir_init());

@*/
{	
  struct tbl* _tbl = malloc(sizeof(struct tbl));
  if(_tbl == 0){
    return 0;
  }
    
  uint16_t* tbl_24 = (uint16_t*) malloc(TBL_24_MAX_ENTRIES * sizeof(uint16_t));
  if(tbl_24 == 0){
    free(_tbl);
    return 0;
  }
    
  uint16_t* tbl_long = (uint16_t*) malloc(TBL_LONG_MAX_ENTRIES * sizeof(uint16_t));
  if(tbl_long == 0){
    free(tbl_24);
    free(_tbl);
    return 0;
  }
   
  //Set every element of the array to INVALID
  fill_invalid(tbl_24, TBL_24_MAX_ENTRIES);
  fill_invalid(tbl_long, TBL_LONG_MAX_ENTRIES);

  //@ assert tbl_24[0..TBL_24_MAX_ENTRIES] |-> repeat_n(nat_of_int(TBL_24_MAX_ENTRIES), INVALID);
  //@ assert tbl_long[0..TBL_LONG_MAX_ENTRIES] |-> repeat_n(nat_of_int(TBL_LONG_MAX_ENTRIES), INVALID);
    
  _tbl->tbl_24 = tbl_24;
  _tbl->tbl_long = tbl_long;
  uint16_t tbl_long_first_index = 0;
  _tbl->tbl_long_index = tbl_long_first_index;
  
  //@ open ushorts(tbl_24, TBL_24_MAX_ENTRIES, ?t_24);
  //@ open ushorts(tbl_long, TBL_LONG_MAX_ENTRIES, ?t_l);
  //@ close ushorts(tbl_24, TBL_24_MAX_ENTRIES, t_24);
  //@ close ushorts(tbl_long, TBL_LONG_MAX_ENTRIES, t_l);
  
  //@ assert t_24 == repeat_n(nat_of_int(TBL_24_MAX_ENTRIES), INVALID);
  //@ assert t_l == repeat_n(nat_of_int(TBL_LONG_MAX_ENTRIES), INVALID);
  //@ assert entry_24_mapping(INVALID) == none;
  
  //@ entries_24_mapping_invalid(entry_24_mapping, t_24);
  //@ entries_long_mapping_invalid(entry_long_mapping, t_l);
  
  //@ map_preserves_length(entry_24_mapping, t_24);
  //@ map_preserves_length(entry_long_mapping, t_l);
  
  //Prove that a list of INVALID is valid
  //@ enforce_map_invalid_is_valid(t_24, valid_entry24);
  //@ enforce_map_invalid_is_valid(t_l, valid_entry_long);
  
  //@ close table(_tbl, tbl_long_first_index, build_tables(t_24, t_l, tbl_long_first_index));

  return _tbl;
}


void tbl_free(struct tbl *_tbl)
//@ requires table(_tbl, _, _);
//@ ensures true;
{
  //@ open table(_tbl, _, _);
  free(_tbl->tbl_24);
  free(_tbl->tbl_long);
  free(_tbl);
}

int tbl_update_elem(struct tbl *_tbl, struct key *_key)
// @ requires table(_tbl, ?long_index, ?dir) &*& key(_key, ?ipv4, ?plen, ?route);
/* @ ensures table(_tbl, long_index,
                  add_rule(dir,
                           init_rule(ipv4, plen, route)
                  )
            )
            &*& key(_key, ipv4, plen, route);
@*/
{
  // @ open key(_key, ipv4, plen, route);
  // @ open table(_tbl, long_index, dir);
  // @ list<option<pair<bool, Z> > > t_24 = dir_tbl24(dir);
  // @  list<option<Z> > t_l = dir_tbl_long(dir);
  
  uint8_t prefixlen = _key->prefixlen;
  uint32_t data = _key->data;
  uint16_t value = _key->route;
  uint16_t *tbl_24 = _tbl->tbl_24;
  uint16_t *tbl_long = _tbl->tbl_long;

  // @ lpm_rule new_rule = init_rule(data, prefixlen, value);

  if(prefixlen > TBL_PLEN_MAX || value > MAX_NEXT_HOP_VALUE){
    // @ close key(_key, ipv4, plen, route);
    // @ close table(_tbl, long_index, dir);
    return -1;
  }

  uint32_t masked_data = data & build_mask_from_prefixlen(prefixlen);

  //If prefixlen is smaller than 24, simply store the value in tbl_24
  if(prefixlen < 24){
    uint32_t first_index = tbl_24_extract_first_index(masked_data);
    uint32_t rule_size = compute_rule_size(prefixlen);
    uint32_t last_index = first_index + rule_size;
    
    // @ list<option<pair<bool, Z> > > t_24_bis = t_24;

    //fill all entries between first index and last index with value
    for(uint32_t i = 0; i < TBL_24_MAX_ENTRIES; i++)
    /* @  invariant 0 <= i &*& i <= TBL_24_MAX_ENTRIES &*&
                   tbl_24[0..i] |-> ?updated &*&
                   //map(entry_24_mapping, updated) == take(i, insert_route_24(t_24, new_rule)) &*&
                   tbl_24[i..TBL_24_MAX_ENTRIES] |-> _;
    
    @*/
    {
      if(i >= first_index && i < last_index){
        // @ new_value(tbl_24, i, TBL_24_MAX_ENTRIES, value);
        // @ update(i, entry_24_mapping(value), t_24_bis);
        // @ assert (nth(i, t_24) == entry_24_mapping(value));
	tbl_24[i] = value;
      }else{
        // @ no_update(tbl_24, i, TBL_24_MAX_ENTRIES);
      }
    }
    // @ assume (t_24 == insert_route_24(t_24, new_rule));
    // @ close key(_key, ipv4, plen, route);
    // @ close table(_tbl, long_index, insert_tbl_24(dir, new_rule));
  } else {
  //If the prefixlen is not smaller than 24, we have to store the value
  //in tbl_long.

    //Check the tbl_24 entry corresponding to the key. If it already has a
    //flag set to 1, use the stored value as base index, otherwise get a new
    //index and store it in the tbl_24
    uint8_t base_index;
    uint32_t tbl_24_index = tbl_24_extract_first_index(data);
      
    uint16_t tbl_24_value = tbl_24[tbl_24_index];
    // @ close entry_24(entry_24_mapping(tbl_24_value));
    bool flag = tbl_24_entry_flag(tbl_24_value);
    bool need_new_index = !flag || tbl_24[tbl_24_index] == INVALID;
      
    if(need_new_index){
      if(_tbl->tbl_long_index >= TBL_LONG_OFFSET_MAX){

        printf("No more available index for tbl_long!\n");fflush(stdout);
        // @ close key(_key, ipv4, plen, route);
        // @ close table(_tbl, 256, dir);
        return -1;
		
      }else{
      //generate next index and store it in tbl_24
        base_index = (uint8_t)(_tbl->tbl_long_index);
        _tbl->tbl_long_index = (uint16_t)(base_index + 1);
            
        tbl_24[tbl_24_index] = tbl_24_entry_set_flag(base_index);
      }      
    }else{
      uint16_t tbl_24_value = tbl_24[tbl_24_index];
      base_index = (uint8_t)(tbl_24_value & 0xFF);
    }
      

    //The last byte in data is used as the starting offset for tbl_long
    //indexes
    uint32_t first_index = tbl_long_extract_first_index(data, prefixlen, base_index);
    uint32_t last_index = first_index + compute_rule_size(prefixlen);

    //Store value in tbl_long entries indexed from value*256+offset up to
    //value*256+255
    for(uint32_t i = 0; i < TBL_LONG_MAX_ENTRIES; i++)
    // @ requires tbl_long[i..TBL_LONG_MAX_ENTRIES] |-> _;
    // @ ensures tbl_long[old_i..TBL_LONG_MAX_ENTRIES] |-> _;
    {  
      if(i >= first_index && i < last_index){
        tbl_long[i] = value;
      }
    }
    // @ close key(_key, ipv4, plen, route);
    // @ close table(_tbl, _, _);
  }
  return 0;
}

int tbl_lookup_elem(struct tbl *_tbl, uint32_t data)
//@ requires table(_tbl, ?long_index, ?dir);
//@ ensures table(_tbl, long_index, dir) &*& result == lpm_dir_24_8_lookup(Z_of_int(data, N32),dir);
{
  //@ open table(_tbl, long_index, dir);
  uint16_t *tbl_24 = _tbl->tbl_24;
  uint16_t *tbl_long = _tbl->tbl_long;
  
  //@ open ushorts(tbl_24, TBL_24_MAX_ENTRIES, ?t_24);
  //@ open ushorts(tbl_long, TBL_LONG_MAX_ENTRIES, ?t_l);
  //@ close ushorts(tbl_24, TBL_24_MAX_ENTRIES, t_24);
  //@ close ushorts(tbl_long, TBL_LONG_MAX_ENTRIES, t_l);
  
  // Prove that the mapping holds, is it enough????
  //@ mapping_holds(entry_24_mapping, t_24, dir_tbl24(dir));
  //@ mapping_holds(entry_long_mapping, t_l, dir_tbl_long(dir));
  
  //@ Z d = Z_of_uint32_custom(data);
  //@ assert d == Z_of_int(data, N32);

  //get index corresponding to key for tbl_24
  uint32_t index = tbl_24_extract_first_index(data);
  //@ assert index == index24_from_ipv4(d);

  uint16_t value = tbl_24[index];
  //Prove that the value retrieved by lookup_tbl_24 is the mapped value retrieved by tbl_24[index]
  //@ lookup_at_index(index, t_24, dir_tbl24(dir), entry_24_mapping);
  //@ assert entry_24_mapping(value) == lookup_tbl_24(index, dir);
  //@ option<pair<bool, Z> > value24 = lookup_tbl_24(index, dir);
  
  //Prove that the retrieved elem is valid
  //@ elem_is_valid(t_24, valid_entry24, index);
	
  if(value != INVALID && tbl_24_entry_flag(value)){
  //the value found in tbl_24 is a base index for an entry in tbl_long,
  //go look at the index corresponding to the key and this base index
  
    //Prove that the value retrieved by lookup_tbl_24 (without the first bit) is 0 <= value <= 0xFF
    //@ valid_next_bucket_long(value, value24);
    
  
    // value must be 0 <= value <= 255
    //@ bitand_limits(data, 0xFF, N32);
    uint16_t index_long = tbl_long_extract_first_index(data, 32, (uint8_t)(value & 0xFF));
    uint16_t value_long = tbl_long[index_long];
    
    //Prove that the value retrieved by lookup_tbl_long is the mapped value retrieved by tbl_24[index]
    //@ lookup_at_index(index_long, t_l, dir_tbl_long(dir), entry_long_mapping);
    //@ assert entry_long_mapping(value_long) == lookup_tbl_long(index_long, dir);
    //@ option<Z> value_l = lookup_tbl_long(index_long, dir);
    
    //Prove that the retrieved elem is valid
    //@ elem_is_valid(t_l, valid_entry_long, index_long);

    //@ close table(_tbl, long_index, dir);
    
    if(value_long == INVALID){
      //@ assert value_l == none;
      //@ invalid_is_none_long(value_long, value_l);
      return INVALID;
    }else{
      //@ valid_next_hop_long(value_long, value_l);
      return value_long;
    }
  } else {
  //the value found in tbl_24 is the next hop, just return it
    //@ close table(_tbl, long_index, dir);
    
    if(value == INVALID){
      //@ invalid_is_none24(value, value24);
      return INVALID;
    }else{;
    //@ valid_next_hop24(value, value24);
      return value;
    }
  }
}
