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

// Address for the blocks

#define GAP_SIZE 50

#define MIDI_PRESETS_START_ADDR 0
#define HMI_START_ADDR (MIDI_PRESETS_START_ADDR+sizeof(persisted_midi_presets_t)+GAP_SIZE)
#define PEDALS_START_ADDR (HMI_START_ADDR+sizeof(pedal_config_t)+GAP_SIZE)


u16 PERSIST_Read16(u16 addr);
u32 PERSIST_Read32(u16 addr);

s32 PERSIST_Write16(u16 addr, u16 value);
s32 PERSIST_Write32(u16 addr, u32 value);

/////////////////////////////////////////////////////////////////////////////
// Reads the EEPROM content during boot
// If EEPROM content isn't valid (magic number mismatch), clear EEPROM
// with default data
/////////////////////////////////////////////////////////////////////////////
s32 PERSIST_Init(u32 mode) {
   s32 status = 0;

   // init EEPROM emulation
   if ((status = EEPROM_Init(0)) < 0) {
#ifdef DEBUG_ENABLED
      DEBUG_MSG("[PRESETS] EEPROM initialisation failed with status %d!\n Attempting to re-format", status);
#endif
      // Reformat
      if ((status = EEPROM_Init(1)) < 0) {
#ifdef DEBUG_ENABLED
         DEBUG_MSG("[PRESETS] EEPROM reformatting failed with status %d!\n", status);
#endif   
      }
   }
   return status;
}

/////////////////////////////////////////////////////////////////////////////
// Persists a block identified by type
/////////////////////////////////////////////////////////////////////////////
s32 PERSIST_ReadBlock(persist_block_t block, unsigned char* pData, u16 length) {
   //Form the serialization in the first 4 bytes from the data
   const unsigned char* dataPtr = pData;
   u32 serializationID = 0;
   for (int i = 0;i < 4;i++) {
      serializationID <<= *dataPtr++;
   }
   // Get the startAddress
   u16 startAddress = PERSIST_GetStartAddress(block);
   u32 eeSerializationID = PERSIST_Read32(startAddress);
   if (serializationID != eeSerializationID) {
      // serializationID doesn't match.  
      DEBUG_MSG("[PERSIST] serializationID: %x doesn't match E^2 id: %x.  Re-formatting EEPROM.  You will need to reboot to write fresh persisted data.", serializationID, eeSerializationID);
      EEPROM_Init(1);
      return -1;
   }
   // Otherwise read the block Big-Endian
   u16 address = startAddress + 4;
   u16 endAddress = startAddress + length;
   while (address < endAddress) {
      u16 word = PERSIST_Read16(address);
      u8 lower = word & 0x00FF;
      u8 upper = word >> 8;
      *pData++ = upper;
      *pData++ = lower;
      address += 2;
   }
   return length;
}

/////////////////////////////////////////////////////////////////////////////
// Persists a block identified by type
/////////////////////////////////////////////////////////////////////////////
s32 PERSIST_StoreBlock(persist_block_t blockType, const unsigned char* pData, u16 length) {
   s32 status = 0;

   u16 startAddress = PERSIST_GetStartAddress(blockType);
   u16 endAddress = startAddress + length;
   while (startAddress < endAddress) {
      u16 word = *pData++;
      word <<= 8;
      word |= *pData++;

      status = PERSIST_Write16(startAddress, word);
      if (status < 0) {
         DEBUG_MSG("PERSIST_StoreBlock: Error writing word to address: 0x%x for block: %d.  Aborting", startAddress, blockType);
         return status;
      }
      startAddress += 2;
   }
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


