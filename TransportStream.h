#ifndef TRANSPORTSTREAM_H
#define TRANSPORTSTREAM_H
#ifdef __GNUG__
#pragma interface
#endif
#ifdef IMPLEMENTING_TRANSPORTSTREAM
static const char *rcsid_TransportStream = "@(#)$Id$";
#endif /* IMPLEMENTING_TRANSPORTSTREAM */
#include <iostream>
#include <vector>
#include <map>
#include <ctime>
#include "TSTypes.h"
#include "Table.h"
#include "TransportPacket.h"
#include "ProgramAssociationSection.h"
#include "ProgramMapSection.h"
#include "EventInformationTable.h"

typedef std::map<uint16, Section*, std::less<uint16> > Pid2SectionMap;

typedef std::map<uint16, uint8, std::less<uint16> > Program2VersionMap;


// Trasnport Stream Event Handling
typedef uint32 TSEvent;
#define TSEvent_Update_ProgramMapTable				((TSEvent)(1<<0))
#define TSEvent_Update_ProgramAssociationTable			((TSEvent)(1<<1))
#define TSEvent_Update_EventInformationTable_Self_Current	((TSEvent)(1<<2))
#define TSEvent_Update_Time					((TSEvent)(1<<3))

class TransportStream {
 public:
   TransportStream();
   ~TransportStream();
   int decode(std::istream *isp);
   void setOption_dump(bool onoff = true);
   void setOption_showProgramInfo(bool onoff = true);
   void setOption_writeTransportStream(const char *filename, bool onoff = true);

 private:
   void dumpPacket(const TransportPacket &packet) const;
   bool loadOption_dump;
   bool loadOption_showProgramInfo;
   bool loadOption_writeTransportStream;
   const char *filename;
   std::ostream *outStream;

 private:
   void loadTable(uint16 pid, const Section &section);
   void loadProgramAssociationTable(const Section &section);
   void loadProgramMapTable(const Section &section, uint16 pid);
   void loadServiceDescriptionTable(const Section &section);
   void loadTimeDateSection(const Section &section);
   void loadEventInformationTable(const Section &section);

   // Contextual Informations
 private:
   TSEvent tsEvent;
   void setTSEvent(TSEvent flag);
   void clearTSEvent();
   std::time_t *latestTimestamp;
   ProgramAssociationSection *latestProgramAssociationTable;
 public:
   bool isActiveTSEvent(TSEvent flag) const;
   std::time_t getLatestTimestamp() const;
   ProgramAssociationSection *getLatestPAT() const;
   std::vector<uint16> programs;
   uint16 getPIDByProgram(uint16 program) const;
   ProgramMapSection *getProgramMapTableByPID(uint16 pid) const;
   std::vector<uint16> programs_updated;  // A list of programs whose PMT was updated
   Program2VersionMap latestEventInformationVersionByProgram;
   
   // Program Specific Information
 private:
   void setPIDByProgram(uint16 program, uint16 pid);
   void setProgramMapSectionByPID(uint16 pid, ProgramMapSection *pmt);
   bool isProgramMapTablePID(uint16 pid) const;
   void clearProgramMapTables();
   bool isPSIComplete() const;
   void showPSI() const;
   std::map<uint16, uint16> program2PID;
   std::map<uint16, uint16> PID2Program;
   std::map<uint16, ProgramMapSection*> PID2ProgramMapSection;

   // Section managements
 private:
   Pid2SectionMap incompleteSections;
   void setIncompleteSection(uint16 pid, Section *sec);
   void unsetIncompleteSection(uint16 pid);
   Section *getIncompleteSection(uint16 pid) const;
};

inline void TransportStream::setOption_dump(bool onoff) {
   loadOption_dump = onoff;
}

inline void TransportStream::setOption_showProgramInfo(bool onoff) {
   loadOption_showProgramInfo = onoff;
}

inline void TransportStream::setOption_writeTransportStream(const char *filename, bool onoff) {
   this->filename = filename;
   loadOption_writeTransportStream = onoff;
}

inline void TransportStream::setTSEvent(TSEvent flag) {
   tsEvent |= flag;
}

inline void TransportStream::clearTSEvent() {
   tsEvent = 0;
}

inline bool TransportStream::isActiveTSEvent(TSEvent flag) const {
   return ((tsEvent & flag) != 0);
}


#endif /* TRANSPORTSTREAM_H */
