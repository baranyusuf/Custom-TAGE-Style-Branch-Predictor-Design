//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <math.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Yusuf Baran";
const char *studentID = "A69040712";
const char *email = "ybaran@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = {"Static", "Gshare",
                         "Tournament", "Custom"};

// define number of bits required for indexing the BHT here.
int ghistoryBits = 15; // Number of bits used for Global History
int bpType;            // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
// TODO: Add your own Branch Predictor data structures here
//
// gshare
uint8_t *bht_gshare;
uint64_t ghistory;

// tournament
uint16_t *lht_tournament;
uint8_t *lp_tournament;
uint8_t *gp_tournament;
uint8_t *cp_tournament;
uint32_t phistory;
uint8_t prediction_local;
uint8_t prediction_global;

//custom
uint32_t bim_idx;
uint32_t CUSTOM_TSIZE[4] = {2048, 1024, 1024, 512};
uint32_t CUSTOM_TMASK[4] = {2047, 1023, 1023, 511};
uint32_t CUSTOM_HLEN[4]    = {4, 12, 36, 60};
uint32_t CUSTOM_TAGBITS[4] = {8, 8, 9, 9};
uint8_t          *custom_bimodal;
custom_TageEntry *custom_TT[4] ;
int idx_bits[4] = {11, 10, 10, 9};
uint64_t custom_ghr      = 0; 
custom_FoldedHist custom_FH[4];
uint32_t custom_use_age = 0;
uint32_t CUSTOM_USE_AGE_PERIOD = (1 << 18);

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//

// gshare functions
void init_gshare()
{
  int bht_entries = 1 << ghistoryBits;
  bht_gshare = (uint8_t *)malloc(bht_entries * sizeof(uint8_t));
  int i = 0;
  for (i = 0; i < bht_entries; i++)
  {
    bht_gshare[i] = WN;
  }
  ghistory = 0;
}

uint8_t gshare_predict(uint32_t pc)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;
  switch (bht_gshare[index])
  {
  case WN:
    return NOTTAKEN;
  case SN:
    return NOTTAKEN;
  case WT:
    return TAKEN;
  case ST:
    return TAKEN;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    return NOTTAKEN;
  }
}

void train_gshare(uint32_t pc, uint8_t outcome)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;

  // Update state of entry in bht based on outcome
  switch (bht_gshare[index])
  {
  case WN:
    bht_gshare[index] = (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    bht_gshare[index] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WT;
    break;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    break;
  }

  // Update history register
  ghistory = ((ghistory << 1) | outcome);
}

void cleanup_gshare()
{
  free(bht_gshare);
}


// tournament functions

void init_tournament()
{
  int lht_entries = 1 << 11;
  int lp_entries = 1 << 11;
  int gp_entries = 1 << 13;
  int cp_entries = 1 << 13;

  lht_tournament = (uint16_t *)malloc(lht_entries * sizeof(uint16_t));
  lp_tournament = (uint8_t *)malloc(lp_entries * sizeof(uint8_t));
  gp_tournament = (uint8_t *)malloc(gp_entries * sizeof(uint8_t));
  cp_tournament = (uint8_t *)malloc(cp_entries * sizeof(uint8_t));
  int i = 0;
  for (i = 0; i < lht_entries; i++)
  {
    lht_tournament[i] = 0;
  }
  for (i = 0; i < lp_entries; i++)
  {
    lp_tournament[i] = WWN;
  }
  for (i = 0; i < gp_entries; i++)
  {
    gp_tournament[i] = WN;
  }
  for (i = 0; i < cp_entries; i++)
  {
    cp_tournament[i] = WN;
  }
  phistory = 0;
}

uint8_t tournament_predict(uint32_t pc)
{
  uint32_t lht_entries = 1 << 11;
  uint32_t pht_entries = 1 << 13;
  uint32_t pc_lower_bits = pc & (lht_entries - 1);
  uint32_t phistory_lower_bits = phistory & (pht_entries - 1);
  uint32_t index_lp = lht_tournament[pc_lower_bits] & (lht_entries - 1);
  uint8_t choice = cp_tournament[phistory_lower_bits];
  
  switch (gp_tournament[phistory_lower_bits])
  {
  case WN:
    prediction_global = NOTTAKEN;
    break;
  case SN:
    prediction_global = NOTTAKEN;
    break;
  case WT:
    prediction_global = TAKEN;
    break;
  case ST:
    prediction_global = TAKEN;
    break;
  default:
    printf("Warning: Undefined state of entry in TOURNAMENT GP1!\n");
    prediction_global = NOTTAKEN;
  }
  
  switch (lp_tournament[index_lp])
  {
  case WWN:
    prediction_local = NOTTAKEN;
    break;
  case WSN:
    prediction_local = NOTTAKEN;
    break;
  case SWN:
    prediction_local = NOTTAKEN;
    break;
  case SSN:
    prediction_local = NOTTAKEN;
    break;
  case WWT:
    prediction_local = TAKEN;
    break;
  case WST:
    prediction_local = TAKEN;
    break;
  case SWT:
    prediction_local = TAKEN;
    break;
  case SST:
    prediction_local = TAKEN;
    break;
  default:
    printf("Warning: Undefined state of entry in TOURNAMENT LP!\n");
    prediction_local = NOTTAKEN;
  }
  
  
  if (choice < 2){
  
  switch (gp_tournament[phistory_lower_bits])
  {
  case WN:
    return NOTTAKEN;
  case SN:
    return NOTTAKEN;
  case WT:
    return TAKEN;
  case ST:
    return TAKEN;
  default:
    printf("Warning: Undefined state of entry in TOURNAMENT GP2!\n");
    return NOTTAKEN;
  }
  }
  else {
  switch (lp_tournament[index_lp])
  {
  case WWN:
    return NOTTAKEN;
  case WSN:
    return NOTTAKEN;
  case SWN:
    return NOTTAKEN;
  case SSN:
    return NOTTAKEN;
  case WWT:
    return TAKEN;
  case WST:
    return TAKEN;
  case SWT:
    return TAKEN;
  case SST:
    return TAKEN;
  default:
    printf("Warning: Undefined state of entry in TOURNAMENT LP!\n");
    return NOTTAKEN;
  }
  }
}

void train_tournament(uint32_t pc, uint8_t outcome)
{
  uint32_t lht_entries = 1 << 11;
  uint32_t pht_entries = 1 << 13;
  uint32_t pc_lower_bits = pc & (lht_entries - 1);
  uint32_t phistory_lower_bits = phistory & (pht_entries - 1);
  uint32_t index_lp = lht_tournament[pc_lower_bits] & (lht_entries - 1);

  switch (gp_tournament[phistory_lower_bits])
  {
  case WN:
    gp_tournament[phistory_lower_bits] = (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    gp_tournament[phistory_lower_bits] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    gp_tournament[phistory_lower_bits] = (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    gp_tournament[phistory_lower_bits] = (outcome == TAKEN) ? ST : WT;
    break;
  default:
    printf("Warning: Undefined state of entry in TOURNAMENT GP3!\n");
    break;
  }
  
  switch (lp_tournament[index_lp])
  {
  case WWN:
    lp_tournament[index_lp] = (outcome == TAKEN) ? WWT : WSN;
    break;
  case WSN:
    lp_tournament[index_lp] = (outcome == TAKEN) ? WWN : SWN;
    break;
  case SWN:
    lp_tournament[index_lp] = (outcome == TAKEN) ? WSN : SSN;
    break;
  case SSN:
    lp_tournament[index_lp] = (outcome == TAKEN) ? SWN : SSN;
    break;
  case WWT:
    lp_tournament[index_lp] = (outcome == TAKEN) ? WST : WWN;
    break;
  case WST:
    lp_tournament[index_lp] = (outcome == TAKEN) ? SWT : WWT;
    break;
  case SWT:
    lp_tournament[index_lp] = (outcome == TAKEN) ? SST : WST;
    break;
  case SST:
    lp_tournament[index_lp] = (outcome == TAKEN) ? SST : SWT;
    break;
  default:
    printf("Warning: Undefined state of entry in TOURNAMENT LP!\n");
  }

  lht_tournament[pc_lower_bits] = ((lht_tournament[pc_lower_bits] << 1) | outcome);
  
  if (prediction_global != prediction_local){
  switch (cp_tournament[phistory_lower_bits])
  {
  case WN:
    cp_tournament[phistory_lower_bits] = (outcome == prediction_global) ? SN : WT;
    break;
  case SN:
    cp_tournament[phistory_lower_bits] = (outcome == prediction_global) ? SN : WN;
    break;
  case WT:
    cp_tournament[phistory_lower_bits] = (outcome == prediction_global) ? WN : ST;
    break;
  case ST:
    cp_tournament[phistory_lower_bits] = (outcome == prediction_global) ? WT : ST;
    break;
  default:
    printf("Warning: Undefined state of entry in TOURNAMENT CP!\n");
    break;
  }
    
  
  }

  // Update history register
  phistory = ((phistory << 1) | outcome);
}


// custom functions

void init_custom() {
  
  uint32_t custom_gshare_size = 1 << 11;
  custom_bimodal = (uint8_t *)malloc(custom_gshare_size * sizeof(uint8_t));
  uint32_t custom_table_size = 1 << 9;
  
  for (int i = 0; i < custom_gshare_size; i++)
  {
    custom_bimodal[i] = WN;
  }
  
  custom_TageEntry *T0 = (custom_TageEntry *)malloc(custom_table_size *4 * sizeof(custom_TageEntry));
  custom_TageEntry *T1 = (custom_TageEntry *)malloc(custom_table_size *2 * sizeof(custom_TageEntry));
  custom_TageEntry *T2 = (custom_TageEntry *)malloc(custom_table_size *2 * sizeof(custom_TageEntry));
  custom_TageEntry *T3 = (custom_TageEntry *)malloc(custom_table_size * sizeof(custom_TageEntry));
  
  for (int i = 0; i < custom_table_size *4; i++)
  {
    T0[i].ctr = 0; 
    T0[i].tag = 0;
    T0[i].u   = 0;
  }
  
  for (int i = 0; i < custom_table_size *2; i++)
  {

    T1[i].ctr = 0;
    T1[i].tag = 0;
    T1[i].u   = 0;

    T2[i].ctr = 0;
    T2[i].tag = 0;
    T2[i].u   = 0;
  }
  
  for (int i = 0; i < custom_table_size; i++)
  {
    T3[i].ctr = 0;
    T3[i].tag = 0;
    T3[i].u   = 0;
  }
  custom_TT[0] = T0; 
  custom_TT[1] = T1; 
  custom_TT[2] = T2; 
  custom_TT[3] = T3; 
  
  for (int i = 0; i < 4; i++) {
    
    custom_FH[i].fold_idx = 0;
    custom_FH[i].fold_tag = 0;
  }
  custom_ghr = 0;
  custom_use_age = 0;
}

uint8_t custom_predict(uint32_t pc) {

  uint32_t custom_gshare_size = 1 << 11;
  uint32_t mask = (custom_gshare_size - 1);
  bim_idx = (pc ^ (pc >> 2)) & mask;
  uint8_t base_ctr = custom_bimodal[bim_idx];
  uint8_t base_pred = (base_ctr >= 2) ? TAKEN : NOTTAKEN;
  
  for (int t = 0; t < 4; t++) {

    int hlen      = CUSTOM_HLEN[t];
    int bits_idx  = idx_bits[t];
    int bits_tag  = CUSTOM_TAGBITS[t];

    uint32_t x = 0; 
    uint32_t y = 0;  
  
    uint32_t mix = (uint32_t)(custom_ghr ^ (custom_ghr >> 7) ^ (custom_ghr << 11));

      for (int i = 0; i < hlen; i++) {
        if ((i % bits_idx) == 0) {
          x ^= (uint32_t)(custom_ghr >> i);
        }
        if ((i % bits_tag) == 0) {
          y ^= (mix >> i);
        }
      }
    

      uint32_t mask_idx = (1u << bits_idx) - 1u;
      x &= mask_idx;

      uint32_t mask_tag = (1u << bits_tag) - 1u;
      y &= mask_tag;

    custom_FH[t].fold_idx = x;
    custom_FH[t].fold_tag = y;
  }
  
  for (int t = 3; t >= 0; --t) {
  
   uint32_t pc_no_lsb   = pc >> 1;                   
   uint32_t folded_idx  = custom_FH[t].fold_idx;   
   uint32_t idx_mask    = CUSTOM_TMASK[t];    

   uint32_t idx = (pc_no_lsb ^ folded_idx) & idx_mask;

   uint32_t raw_tag32   = pc ^ (pc >> 3);            
   uint16_t tag_mask    = (uint16_t)((1 << CUSTOM_TAGBITS[t]) - 1);
   uint16_t tag         = (uint16_t)(raw_tag32 & tag_mask);

   tag ^= (uint16_t)custom_FH[t].fold_tag;             

   if (custom_TT[t][idx].tag == tag) {
      int8_t pctr = custom_TT[t][idx].ctr;
      uint8_t prov_pred = (pctr >= 0) ? TAKEN : NOTTAKEN;
      return prov_pred;
   } 
  }
  return base_pred;
}


void train_custom(uint32_t pc, uint8_t outcome) {

  int bctr = custom_bimodal[bim_idx];

  if (outcome == TAKEN) {
    if (custom_bimodal[bim_idx] < 3) custom_bimodal[bim_idx] += 1;
  } else {
    if (custom_bimodal[bim_idx] > 0) custom_bimodal[bim_idx] -= 1;
  }

  int prov_t   = -1;
  int prov_idx = -1; 

  for (int t = 3; t >= 0; t--) {
    uint32_t idx = ((pc >> 1) ^ custom_FH[t].fold_idx) & CUSTOM_TMASK[t];

    uint16_t tag = (uint16_t)((pc ^ (pc >> 3)) & ((1 << CUSTOM_TAGBITS[t]) - 1));
    tag ^= (uint16_t)custom_FH[t].fold_tag;

    if (custom_TT[t][idx].tag == tag) {
      if (prov_t < 0) {
        prov_t   = t;
        prov_idx = (int)idx;
      } 
    }
  }

  if (prov_t >= 0) {
    int pctr = custom_TT[prov_t][prov_idx].ctr;
    uint8_t p_pred = (pctr >= 0) ? TAKEN : NOTTAKEN;

    if (outcome == TAKEN) {
      if (pctr < +3) pctr++;
    } else {
      if (pctr > -4) pctr--;
    }
    custom_TT[prov_t][prov_idx].ctr = (int8_t)pctr;

    if ((p_pred == outcome) && (pctr >= +2 || pctr <= -3)) {
      custom_TT[prov_t][prov_idx].u = 1;
    } else if ((p_pred != outcome) && (pctr >= +2 || pctr <= -3)) {
      custom_TT[prov_t][prov_idx].u = 0;
    }

    bool need_alloc = false;

    if (p_pred != outcome) {
      need_alloc = true;
    } 

    if (need_alloc) {
      int allocated = 0;
      for (int t = prov_t + 1; t < 4 && allocated < 2; t++) {
        uint32_t idx = ((pc >> 1) ^ custom_FH[t].fold_idx) & CUSTOM_TMASK[t];

        uint16_t tag = (uint16_t)((pc ^ (pc >> 3)) & ((1 << CUSTOM_TAGBITS[t]) - 1U));
        tag ^= (uint16_t)custom_FH[t].fold_tag;

        if (custom_TT[t][idx].tag != tag || custom_TT[t][idx].u == 0) {
          custom_TT[t][idx].tag = tag;
          custom_TT[t][idx].ctr = (outcome == TAKEN) ? 0 : -1; 
          custom_TT[t][idx].u   = 0;
          allocated++;
        }
      }
    }

  } else {
    int allocated = 0;
    for (int t = 0; t < 4 && allocated < 2; t++) {
      uint32_t idx = ((pc >> 1) ^ custom_FH[t].fold_idx) & CUSTOM_TMASK[t];

      uint16_t tag = (uint16_t)((pc ^ (pc >> 3)) & ((1U << CUSTOM_TAGBITS[t]) - 1U));
      tag ^= (uint16_t)custom_FH[t].fold_tag;

      if (custom_TT[t][idx].u == 0) {
        custom_TT[t][idx].tag = tag;
        custom_TT[t][idx].ctr = (outcome == TAKEN) ? 0 : -1;
        custom_TT[t][idx].u   = 0;
        allocated++;
      }
    }
  }

  custom_use_age++;
  if ((custom_use_age & (CUSTOM_USE_AGE_PERIOD - 1)) == 0) {
    for (int t = 0; t < 4; t++) {
      for (int i = 0; i < CUSTOM_TSIZE[t]; i++) {
        custom_TT[t][i].u = 0;
      }
    }
  }

  custom_ghr = (custom_ghr << 1) | (uint64_t)(outcome & 1);

  uint64_t mask = 0;
  for (int i = 0; i < 60; i++) {
      mask |= 1 << i;
  }
  custom_ghr &= mask;

}


void init_predictor(){
  switch (bpType)
  {
  case STATIC:
    break;
  case GSHARE:
    init_gshare();
    break;
  case TOURNAMENT:
    init_tournament();
    break;
  case CUSTOM:
    init_custom();
    break;
  default:
    break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint32_t make_prediction(uint32_t pc, uint32_t target, uint32_t direct){

  // Make a prediction based on the bpType
  switch (bpType)
  {
  case STATIC:
    return TAKEN;
  case GSHARE:
    return gshare_predict(pc);
  case TOURNAMENT:
    return tournament_predict(pc);
  case CUSTOM:
    return custom_predict(pc);
  default:
    break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//

void train_predictor(uint32_t pc, uint32_t target, uint32_t outcome, uint32_t condition, uint32_t call, uint32_t ret, uint32_t direct)
{
  if (condition)
  {
    switch (bpType)
    {
    case STATIC:
      return;
    case GSHARE:
      return train_gshare(pc, outcome);
    case TOURNAMENT:
      return train_tournament(pc, outcome);
    case CUSTOM:
      return train_custom(pc, outcome);
    default:
      break;
    }
  }
}
