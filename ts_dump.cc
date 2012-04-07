/**
 *
 *
 */
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <getopt.h>
#include "TransportStream.h"
#include "StdLogger.h"

static char rcsid[] = "@(#)$Id$";

Logger *logger;

static void usage(const char *argv0) {
   std::cerr << "usage: " << argv0 << " [options]" << " -i input.ts [-o output.ts]" << std::endl;
   std::cerr << " options:" << std::endl;
   std::cerr << "   -v             print verbose message" << std::endl;
   std::cerr << "   -s             show program information" << std::endl;
   std::cerr << "   -d             print debug information" << std::endl;
   std::cerr << "   -p program_id  output only specified program" << std::endl;
}

/*
 * Managing active PIDs
 */
typedef std::map<uint16, int, std::less<int> > PIDMap;
class ActivePID {
public:
   bool isActive(uint16 pid) const;
   void activate(uint16 pid);
   void deactivate(uint16 pid);
   void reset();
private:
   PIDMap pids;
};

inline bool ActivePID::isActive(uint16 pid) const {
   PIDMap::const_iterator itr = pids.find(pid);
   if (itr == pids.end()) return false;
   return true;
}

void ActivePID::activate(uint16 pid) {
   pids[pid] = 1;
}

void ActivePID::deactivate(uint16 pid) {
   PIDMap::iterator itr = pids.find(pid);
   if (itr == pids.end()) return;
   pids.erase(itr);
}

void ActivePID::reset() {
   pids.clear();
}

/*
 * Callback function for PMT parse
 */
void RegisterPIDFromPMT(uint16 pid, const char *tagstr, void *dtp) {
   ActivePID *pids = (ActivePID *)dtp;
   pids->activate(pid);
}


/*
 * main
 */
int main(int argc, char *argv[]) {
   int i;
   char *argv0;
   bool opt_v = false;
   bool opt_d = false;
   bool opt_s = false;
   char *filename_in = NULL;
   char *filename_out = NULL;
   int program_id = -1;

   // Parse command line options.
   int option_char;
   while ((option_char = getopt(argc, argv, "vdsi:o:p:")) != -1) {
      switch (option_char) {
      case 'v':
	 opt_v = true;
	 break;
      case 'd':
	 opt_d = true;
	 opt_v = true;
	 break;
      case 's':
	 opt_s = true;
	 break;
      case 'i':
	 filename_in = optarg;
	 break;
      case 'o':
	 filename_out = optarg;
	 break;
      case 'p':
	 program_id = atoi(optarg);
	 break;
      case 'h':
      default:
	 usage(argv[0]);
	 return 1;
      }
   }
   argv0 = argv[0];
   argc -= optind;
   argv += optind;
   if (argc != 0 || filename_in == NULL) {
      usage(argv0);
      return 1;
   }

   int logmode = LOGGER_ERROR | LOGGER_WARNING;
   if (opt_d) {
      logmode |= LOGGER_DEBUG;
   }
   StdLogger lgr(logmode);
   logger = &lgr;

   // Open files
   std::ifstream ifs(filename_in);
   if (!ifs) {
      std::cerr << argv0 << ": error openning file " << filename_in << std::endl;
      return 1;
   }
   BufferedInputStream *bisp = new BufferedInputStream(&ifs);
   std::ofstream ofs;
   if (filename_out != NULL) {
      ofs.open(filename_out);
      if (!ofs) {
	 std::cerr << argv0 << ": error openning file " << filename_out << std::endl;
	 return 1;
      }
   }

   // Input stream
   TransportStream ts;
   ts.setOption_dump(opt_v);
   ts.setOption_showProgramInfo(opt_s);
   ts.setOption_writeTransportStream(NULL, false);
   
   ifs.exceptions(std::ios::badbit);
   ofs.exceptions(std::ios::badbit);
   try {
      ActivePID activePIDs;
      activePIDs.activate(PID_ProgramAssociationTable);
      activePIDs.activate(PID_TimeDateSection);
      
      while (!bisp->eof()) {

	 /*----------------------------
	  * Decode input stream
	  */
	 int len = ts.decode(bisp);
	 if (len <= 0) {
	    if (bisp->eof()) break;
	    logger->error("main: failed to decode input stream");
	    return 1;
	 }

	 if (filename_out && ts.packet) {
	    /*----------------------------
	     * Filterings
	     */
	    bool writePacket = true;

	    if (program_id >= 0) {
	       if (ts.isActiveTSEvent(TSEvent_Update_ProgramAssociationTable)) {
		  ProgramAssociationSection *pat = ts.getLatestPAT();
		  int numPrograms = pat->numPrograms();
		  for (int i = 0; i < numPrograms; i++) {
		     uint16 pno = pat->program_number(i);
		     if (pno == 0 || pno == program_id) {
			activePIDs.activate(pat->program_map_PID(i));
		     }
		  }
	       }
	       if (ts.isActiveTSEvent(TSEvent_Update_ProgramMapTable)) {
		  for (int i = 0; i < ts.programs_updated.size(); i++) {
		     uint16 pno = ts.programs_updated[i];
		     if (pno == 0 || pno == program_id) {
			uint16 pmt_pid = ts.getPIDByProgram(ts.programs_updated[i]);
			ProgramMapSection *pmt = ts.getProgramMapTableByPID(pmt_pid);
			assert(pmt != NULL);
			assert(pmt->isComplete());
			pmt->for_all_streams(RegisterPIDFromPMT, &activePIDs);
		     }
		  }
	       }

	       if (!activePIDs.isActive(ts.packet->PID())) {
		  writePacket = false;
	       }
	    }

	    /*----------------------------
	     * Write output stream
	     */
	    if (writePacket) {
	       ts.packet->write(&ofs);
	    }
	 }

	 /*----------------------------
	  * Post processings
	  */
	 if (ts.isActiveTSEvent(TSEvent_Update_ProgramMapTable)) {
	    ts.programs_updated.clear();
	 }
      }
   } catch (const std::ios::failure& error) {
      std::cerr << "I/O exception: " << error.what() << std::endl;
      return 1;
   }
   
   if (filename_out) {
      ofs.close();
   }
   ifs.close();

   return 0;
}
