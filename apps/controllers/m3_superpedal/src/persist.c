/*
 * Persistance for M3 SuperPedal
 *  Scott Rush 2024
 */

 /////////////////////////////////////////////////////////////////////////////
 // Include files
 /////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include <eeprom.h>
#include <midi_router.h>
#include "persist.h"


#include "midi_presets.h"
#include "pedals.h"
#include "hmi.h" 
#include "arp.h"
#include "arp_hmi.h"

#undef DEBUG

#ifndef DEBUG_MSG
# define DEBUG_MSG MIOS32_MIDI_SendDebugMessage
#endif

// Address for the blocks.  All in 16-bit word size

#define PRESETS_ADDR_ROUTER_BEGIN 0
#define MIDI_PRESETS_START_ADDR (PRESETS_ADDR_ROUTER_BEGIN+(MIDI_ROUTER_NUM_NODES*4))
#define HMI_START_ADDR (MIDI_PRESETS_START_ADDR+sizeof(persisted_midi_presets_t)/2)
#define PEDALS_START_ADDR (HMI_START_ADDR+sizeof(persisted_hmi_settings_t)/2)
#define ARP_START_ADDR (PEDALS_START_ADDR+sizeof(persisted_pedal_confg_t)/2)
#define ARP_HMI_START_ADDR (ARP_START_ADDR+sizeof(persisted_arp_data_t)/2)

u16 PERSIST_Read16(u16 addr);
u32 PERSIST_Read32(u16 addr);

s32 PERSIST_Write16(u16 addr, u16 value);
s32 PERSIST_Write32(u16 addr, u32 value);
u32 PERSIST_ParseSerializationID(const unsigned char* pData);

/////////////////////////////////////////////////////////////////////////////
// Initializes persistence.
// mode:  0 for regular init, 1 for forced re-format.
/////////////////////////////////////////////////////////////////////////////
s32 PERSIST_Init(u32 mode) {
   s32 status = 0;

   // init EEPROM emulation
   if ((status = EEPROM_Init(mode)) < 0) {
#ifdef DEBUG
      DEBUG_MSG("PERSIST_Init: EEPROM initialisation failed with status %d!\n", status);
#endif

      /**
           // Reformat
           if ((status = EEPROM_Init(1)) < 0) {
     #ifdef DEBUG
              DEBUG_MSG("[PRESETS] EEPROM reformatting failed with status %d!\n", status);
     #endif
           }
        **/
   }
   else if (mode > 0) {
      // E2 just got reformatted, write the MidiRouter settings
      PERSIST_StoreMIDIRouter();
   }
   else {
      // Restore midi_router settings.  Need to call from here to avoid modifying the midi_router
      // module that doesn't support persist.c
      PERSIST_RestoreMidiRouter();
   }

   return status;
}

/////////////////////////////////////////////////////////////////////////////
// Persists a block identified by type
// block:  block type persist_block_t enum - used internally to compute the starting EEPROM addresses
// pData:  point to serialized client block
// length: length in bytes
//
// First 4 bytes of pData must be the expected serializationID in Little-Endian (native ARM mode)  If not found
// in EEPROM, then an error will be returned.
/////////////////////////////////////////////////////////////////////////////
s32 PERSIST_ReadBlock(persist_block_t block, unsigned char* pData, u16 length) {
   //Form the serialization in the first 4 bytes from the data.  Note that it is in the stream little-endian
   // even though it will be persisted to E2 big-endian
   u32 serializationID = PERSIST_ParseSerializationID(pData);
   // Get the startAddress
   u16 startAddress = PERSIST_GetStartAddress(block);
   u32 eeSerializationID = PERSIST_Read32(startAddress);
   if (serializationID != eeSerializationID) {
      // serializationID doesn't match.  
      DEBUG_MSG("PERSIST_ReadBlock: serializationID: 0x%X doesn't match E^2 id: 0x%X.", serializationID, eeSerializationID);
      return -1;
   }
   // Otherwise read the block Big-Endian words and convert to char stream.  Remember E2 addresses are in 16-bit Word length
   u16 address = startAddress;
   u16 byteCount = 0;
   while (byteCount < length) {
      u16 word = PERSIST_Read16(address);
      u8 lower = word & 0x00FF;
      u8 upper = 0x00FF & (word >> 8);
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


#ifdef DEBUG
   DEBUG_MSG("PERSIST_StoreBlock called:  blockType %d, length %d, startAddr=%d", blockType, length, wordAddress);
#endif
   // Get serializationID and write out with the helper. It has to write out big-endian. Rest goes out
   // as stream
   u32 serializationID = PERSIST_ParseSerializationID(pData);
   PERSIST_Write32(wordAddress, serializationID);
   wordAddress += 2;

   // Now do rest after serialiation ID
   u16 byteOffset = 4;
   while (byteOffset < length) {
      // Form big-endian word
      u16 word = 0x00FF & *(pData + byteOffset);
      word <<= 8;
      byteOffset++;

      u16 lower = *(pData + byteOffset);
      word |= (0x00FF & lower);
      byteOffset++;

      // Do a read and only write the word if different
      u16 readWord = PERSIST_Read16(wordAddress);
      status = 0;
      if (readWord != word) {
         status = PERSIST_Write16(wordAddress, word);
      }
      /* #ifdef DEBUG
            u16 readBackWord = 0;
            readBackWord = PERSIST_Read16(wordAddress);
            DEBUG_MSG("PERSIST_StoreBlock:  Writing word %d to address %d, readBackWord=%d", word, wordAddress, readBackWord);
      #endif */
      if (status < 0) {
         DEBUG_MSG("PERSIST_StoreBlock: Error writing word to wordAddress: %d for block: %d.  Aborting", wordAddress, blockType);
         return status;
      }
      wordAddress++;
   }
   /* #ifdef DEBUG
      // Do a readback check if DEBUG
      unsigned char checkDataBuffer[length];
      // Copy in the serialization ID
      for(int i=0;i <= 3;i++){
         checkDataBuffer[i] = *(pData+i);
      }
      PERSIST_ReadBlock(blockType, checkDataBuffer, length);
      u8 errorcount = 0;
      for (int i = 0;i < length;i++) {
         DEBUG_MSG("PERSIST_StoreBlock:  Readback. offset=%d written=%u read=%u", i, *(pData + i), checkDataBuffer[i]);
         if (checkDataBuffer[i] != *(pData + i)) {
            errorcount++;
         }
      }
      if (errorcount > 0) {
         DEBUG_MSG("PERSIST_StoreBlock:  Failed readback count=%d", errorcount);
      }
   #endif */
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
   case PERSIST_ARP_BLOCK:
      startAddress = ARP_START_ADDR;
      break;
   case PERSIST_ARP_HMI_BLOCK:
      startAddress = ARP_HMI_START_ADDR;
      break;
   default:
      DEBUG_MSG("Invalid block type:%d", blockType);
      return 0xFFFF;  // error

   }
   return startAddress;
}

u32 PERSIST_ParseSerializationID(const unsigned char* pData) {
   u32 serializationID = 0;
   for (int i = 3;i >= 0;i--) {
      u8 byte = *(pData + i);
      serializationID |= (0x000000FF & byte);
      if (i > 0)
         serializationID <<= 8;
#ifdef DEBUG
      DEBUG_MSG("PERSIST_ParseSerializationID: forming byte=%u serializationID=0x%X", byte, serializationID);
#endif
   }
   return serializationID;
}
/////////////////////////////////////////////////////////////////////////////
// Help functions to read a value from EEPROM:  Big-endian coding
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

/////////////////////////////////////////////////////////////////////////////
// Helper to restore  the Midi Router settings since the midi_router module
// doesn't support PERSIST.C.
/////////////////////////////////////////////////////////////////////////////
void PERSIST_RestoreMidiRouter() {
   u8 node;
   midi_router_node_entry_t* n = &midi_router_node[0];
   for (node = 0; node < MIDI_ROUTER_NUM_NODES; ++node, ++n) {
      u16 cfg1 = PERSIST_Read16(PRESETS_ADDR_ROUTER_BEGIN + node * 2 + 0);
      u16 cfg2 = PERSIST_Read16(PRESETS_ADDR_ROUTER_BEGIN + node * 2 + 1);

      // default setup
      if (!cfg1 && !cfg2) {
         n->src_port = USB0;
         n->src_chn = 0;
         n->dst_port = UART0;
         n->dst_chn = 17; // All
      }
      else {
         n->src_port = (cfg1 >> 0) & 0xff;
         n->src_chn = (cfg1 >> 8) & 0xff;
         n->dst_port = (cfg2 >> 0) & 0xff;
         n->dst_chn = (cfg2 >> 8) & 0xff;
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
// Helper to store  the Midi Router settings since the midi_router module
// doesn't support PERSIST.C.
/////////////////////////////////////////////////////////////////////////////
s32 PERSIST_StoreMIDIRouter() {

   u8 node;
   s32 status = 0;
   midi_router_node_entry_t* n = &midi_router_node[0];
   for (node = 0; node < MIDI_ROUTER_NUM_NODES; ++node, ++n) {
      u16 cfg1 = n->src_port | ((u16)n->src_chn << 8);
      u16 cfg2 = n->dst_port | ((u16)n->dst_chn << 8);

      status |= PERSIST_Write16(PRESETS_ADDR_ROUTER_BEGIN + node * 2 + 0, cfg1);
      status |= PERSIST_Write16(PRESETS_ADDR_ROUTER_BEGIN + node * 2 + 1, cfg2);
   }
   return status;
}