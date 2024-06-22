/*
 * Persistance for M3 SuperPedal
 *  Scott Rush 2024
 */

 /////////////////////////////////////////////////////////////////////////////
 // Include files
 /////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include <eeprom.h>

#include "persist.h"

#define DEBUG_ENABLED

#ifndef DEBUG_MSG
# define DEBUG_MSG MIOS32_MIDI_SendDebugMessage
#endif

// Address for the blocks.  All in 16-bit word size
#define GAP_SIZE 50
#define MIDI_PRESETS_START_ADDR 0
#define HMI_START_ADDR (MIDI_PRESETS_START_ADDR+sizeof(persisted_midi_presets_t)/2+GAP_SIZE)
#define PEDALS_START_ADDR (HMI_START_ADDR+sizeof(pedal_config_t)/2+GAP_SIZE)


u16 PERSIST_Read16(u16 addr);
u32 PERSIST_Read32(u16 addr);

s32 PERSIST_Write16(u16 addr, u16 value);
s32 PERSIST_Write32(u16 addr, u32 value);

/////////////////////////////////////////////////////////////////////////////
// Reads the EEPROM content during boot
// If EEPROM content isn't valid (magic number mismatch), clear EEPROM
// with default data
// mode:  0 for regular init, 1 for forced re-format.
/////////////////////////////////////////////////////////////////////////////
s32 PERSIST_Init(u32 mode) {
   s32 status = 0;

   // init EEPROM emulation
   if ((status = EEPROM_Init(mode)) < 0) {
#ifdef DEBUG_ENABLED
      DEBUG_MSG("PERSIST_Init: EEPROM initialisation failed with status %d!\n", status);
#endif
 
 /**
      // Reformat
      if ((status = EEPROM_Init(1)) < 0) {
#ifdef DEBUG_ENABLED
         DEBUG_MSG("[PRESETS] EEPROM reformatting failed with status %d!\n", status);
#endif   
      }
   **/
   }
   
   return status;
}

/////////////////////////////////////////////////////////////////////////////
// Persists a block identified by type
// block:  block type persist_block_t enum - used internally to compute the starting EEPROM addresses
// pData:  point to serialized client block
// length: length in bytes
//
// First 4 bytes of pData must be the expected serializationID in Little-Endian  If not found
// in EEPROM, then an error will be returned.
/////////////////////////////////////////////////////////////////////////////
s32 PERSIST_ReadBlock(persist_block_t block, unsigned char* pData, u16 length) {
   //Form the serialization in the first 4 bytes from the data little undian
   u32 serializationID = 0;
   for (int i = 3;i >= 0;i--) {
      u8 byte = *(pData+i);
      serializationID |= (0x000000FF && byte);
      serializationID <<= 8;   
      DEBUG_MSG("PERSIST_ReadBlock: forming serializationID=0x%X",serializationID);
   }
   // Get the startAddress
   u16 startAddress = PERSIST_GetStartAddress(block);
   u32 eeSerializationID = PERSIST_Read32(startAddress);
   if (serializationID != eeSerializationID) {
      // serializationID doesn't match.  
      DEBUG_MSG("PERSIST_ReadBlock: serializationID: 0x%X doesn't match E^2 id: 0x%X.",serializationID, eeSerializationID);
      return -1;
   }
   // Otherwise read the block Big-Endian.  Remember E2 addresses are in 16-bit Word length
   u16 address = startAddress;
   u16 byteCount = 0;                 
   while (byteCount < length) {
      u16 word = PERSIST_Read16(address);
      u8 lower = word & 0x00FF;
      u8 upper = word >> 8;
      *pData++ = upper;
      *pData++ = lower;

      address += 1;
      byteCount += 2;
   }
   return length;
}

/////////////////////////////////////////////////////////////////////////////
// Persists a block identified by type
/////////////////////////////////////////////////////////////////////////////
s32 PERSIST_StoreBlock(persist_block_t blockType, const unsigned char* pData, u16 length) {
   s32 status = 0;

   u16 wordAddress = PERSIST_GetStartAddress(blockType);
#ifdef DEBUG_ENABLED
   DEBUG_MSG("PERSIST_StoreBlock called:  blockType %d, length %d, startAddr=%d",blockType,length,wordAddress);
#endif
   u16 byteOffset = 0;
   while (byteOffset < length) {
      // Form little-endian word
      u16 word = *(pData+byteOffset);
      byteOffset++;
      
      u8 upper = *(pData+byteOffset);
      upper <<= 8;
      word |= (0xFF00 & upper);
      byteOffset++;

      status = PERSIST_Write16(wordAddress, word);
      if (status < 0) {
         DEBUG_MSG("PERSIST_StoreBlock: Error writing word to wordAddress: %d for block: %d.  Aborting", wordAddress, blockType);
         return status;
      }
      wordAddress++;
   }
#ifdef DEBUG_ENABLED
   // Do a readback check if DEBUG_ENABLED
   unsigned char checkDataBuffer[length];
   PERSIST_ReadBlock(blockType,checkDataBuffer,length);
   u8 errorcount = 0;
   for(int i=0;i < length;i++){
      DEBUG_MSG("PERSIST_StoreBlock:  Readback. offset=%d written=%u read=%u",i,*(pData+i),checkDataBuffer[i]);
      if (checkDataBuffer[i] != *(pData+i)){
         errorcount++;
      }
   }
   if (errorcount > 0){
      DEBUG_MSG("PERSIST_StoreBlock:  Failed readback count=%d",errorcount);
   }
#endif
   return status;
}
/////////////////////////////////////////////////////////////////////////////
// helper to get start address
/////////////////////////////////////////////////////////////////////////////
u16 PERSIST_GetStartAddress(persist_block_t blockType) {
   u16 startAddress = 0;
   switch (blockType) {
   case PERSIST_MIDI_PRESETS_BLOCK:
      startAddress = MIDI_PRESETS_START_ADDR;
      break;
   case PERSIST_HMI_BLOCK:
      startAddress = HMI_START_ADDR;
      break;
   case PERSIST_PEDALS_BLOCK:
      startAddress = PEDALS_START_ADDR;
      break;
   default:
      DEBUG_MSG("Invalid block type:%d", blockType);
      return 0xFFFF;  // error

   }
   return startAddress;
}
/////////////////////////////////////////////////////////////////////////////
// Help functions to read a value from EEPROM (big endian coding!)
/////////////////////////////////////////////////////////////////////////////
u16 PERSIST_Read16(u16 addr) {
   return EEPROM_Read(addr);
}

u32 PERSIST_Read32(u16 addr) {
   return ((u32)EEPROM_Read(addr) << 16) | EEPROM_Read(addr + 1);
}

/////////////////////////////////////////////////////////////////////////////
// Help function to write a value into EEPROM (big endian coding!)
/////////////////////////////////////////////////////////////////////////////
s32 PERSIST_Write16(u16 addr, u16 value) {
   return EEPROM_Write(addr, value);
}

s32 PERSIST_Write32(u16 addr, u32 value) {
   return EEPROM_Write(addr, (value >> 16) & 0xffff) | EEPROM_Write(addr + 1, (value >> 0) & 0xffff);
}


