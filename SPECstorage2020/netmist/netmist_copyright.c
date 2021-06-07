/**
 *  @copyright
 *  Copyright (c) 2002-2020 by Iozone.org
 *      All rights reserved.
 *              Iozone.org
 *              7417 Crenshaw Dr.
 *              Plano, TX 75025
 *
 *      This product contains benchmarks acquired from several sources who
 *      understand and agree with Iozone's goal of creating fair and objective
 *      benchmarks to measure computer performance.
 *
 *      This copyright notice is placed here only to protect Iozone.org in the
 *      event the source is misused in any manner that is contrary to the
 *      spirit, the goals and the intent of Iozone.org
 * 
 *      Founder, author, maintainer: Don Capps
 *      Author: Udayan Bapat, NetApp Inc.
 */
#include "./copyright.txt"
#include "./license.txt"

#include <stdio.h>

#include "netmist_version.h"
#include "netmist_utils.h"
#include "netmist_logger.h"

char Author[] = "Don Capps. Email: capps@iozone.org";
char Maintainer[] = "Don Capps. Email: capps@iozone.org";
char Contributors[] = "Don Capps, Carol Capps, Darren Sawyer, Jerry Lohr,";
char Contributors2[] = "George Dowding, Gary Little, Terry Capps,";
char Contributors3[] = "Robin Miller, Sorin Faibish, Raymond Wang,";
char Contributors4[] = "Tanmay Waghmare, Yansheng Zhang, Vernon Miller,";
char Contributors5[] = "Nick Principe, Zach Jones, Udayan Bapat";

#if defined(LITE)
extern char testname[];
extern char short_testname[];
#endif

void
print_copyright (void)
{
#if defined(LITE)
    printf ("\n");
    printf
	("     *************************************************************************\n");
    printf ("     |     %-30s Release         %24s        |\n", testname,
	    git_version);
    printf
	("     |                                    by                                 |\n");
    printf ("     |     Author:       %-50s  |\n", Author);
    printf ("     |     Maintainer:   %-50s  |\n", Maintainer);
    printf ("     |     Contributors: %-50s  |\n", Contributors);
    printf ("     |                   %-50s  |\n", Contributors2);
    printf ("     |                   %-50s  |\n", Contributors3);
    printf ("     |                   %-50s  |\n", Contributors4);
    printf ("     |                   %-50s  |\n", Contributors5);
    printf
	("     *************************************************************************\n");
    printf ("      %s %s\n\n", short_testname, git_version);
    printf ("\n");
    printf ("     This program contains contributions from Iozone.org.\n");
    printf ("\n");
    printf ("     Copyright (C) 2002-2020, Don Capps\n");
    printf ("     All rights reserved.\n");
    printf ("\n");
    printf
	("     This program contains a 64-bit version of Mersenne Twister pseudorandom \n");
    printf ("     number generator.\n");
    printf ("\n");
    printf
	("     Copyright (C) 2004, Makoto Matsumoto and Takuji Nishimura\n");
    printf ("     All rights reserved.\n");
    printf ("\n");
    printf
	("     Redistribution and use in source and binary forms, with or without\n");
    printf
	("     modification, are permitted provided that the following conditions \n");
    printf ("     are met:\n");
    printf ("\n");
    printf
	("     1. Redistributions of source code must retain the above copyrights\n");
    printf
	("        notices, this list of conditions and the following disclaimer.\n");
    printf
	("     2. Redistributions in binary form must reproduce the above copyright\n");
    printf
	("        notices, this list of conditions and the following disclaimer in \n");
    printf
	("        the documentation and/or other materials provided with the \n");
    printf ("        distribution.\n");
    printf
	("     3. The names of its contributors may not be used to endorse or \n");
    printf
	("        promote products derived from this software without specific prior \n");
    printf ("        written permission.\n");
    printf ("\n");
    printf
	("     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n");
    printf
	("     \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n");
    printf
	("     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n");
    printf
	("     A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT \n");
    printf
	("     OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, \n");
    printf
	("     SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT \n");
    printf
	("     LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, \n");
    printf
	("     DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\n");
    printf
	("     THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT \n");
    printf
	("     (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n");
    printf
	("      OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n");
    printf ("\n");
    fflush (stdout);
#endif
#if defined(SPEC_DIST)
    printf ("\n");
    printf ("     SPECstorage(TM) Solution 2020 Release %24s\n", git_version);
    printf ("\n");
    printf
	("     This product contains benchmarks acquired from several sources who\n");
    printf
	("     understand and agree with SPEC's goal of creating fair and objective\n");
    printf ("     benchmarks to measure computer performance.\n");
    printf ("\n");
    printf
	("     This copyright notice is placed here only to protect SPEC in the\n");
    printf
	("     event the source is misused in any manner that is contrary to the\n");
    printf ("     spirit, the goals and the intent of SPEC.\n");
    printf ("\n");
    printf
	("     The source code is provided to the user or company under the license\n");
    printf
	("     agreement for the SPEC Benchmark Suite for this product.\n");
    printf ("\n");
    printf
	("     -----------------------------------------------------------------------\n");
    printf ("     This program contains contributions from Iozone.org.\n");
    printf ("\n");
    printf ("     Copyright (C) 2002-2020, Don Capps\n");
    printf ("     All rights reserved.\n");
    printf ("\n");
    printf
	("     This program contains a 64-bit version of Mersenne Twister pseudorandom \n");
    printf ("     number generator.\n");
    printf ("\n");
    printf
	("     Copyright (C) 2004, Makoto Matsumoto and Takuji Nishimura\n");
    printf ("     All rights reserved.\n");
    printf ("\n");
    printf
	("     Redistribution and use in source and binary forms, with or without\n");
    printf
	("     modification, are permitted provided that the following conditions \n");
    printf ("     are met:\n");
    printf ("\n");
    printf
	("     1. Redistributions of source code must retain the above copyright\n");
    printf
	("        notices, this list of conditions and the following disclaimer.\n");
    printf
	("     2. Redistributions in binary form must reproduce the above copyright\n");
    printf
	("        notices, this list of conditions and the following disclaimer in \n");
    printf
	("        the documentation and/or other materials provided with the \n");
    printf ("        distribution.\n");
    printf
	("     3. The names of its contributors may not be used to endorse or \n");
    printf
	("        promote products derived from this software without specific prior \n");
    printf ("        written permission.\n");
    printf ("\n");
    printf
	("     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n");
    printf
	("     \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n");
    printf
	("     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n");
    printf
	("     A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT \n");
    printf
	("     OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, \n");
    printf
	("     SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT \n");
    printf
	("     LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, \n");
    printf
	("     DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\n");
    printf
	("     THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT \n");
    printf
	("     (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n");
    printf
	("     OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n");
    printf
	("     -----------------------------------------------------------------------\n");
    printf ("\n");
    fflush (stdout);
#endif
#if defined(PRO)
    printf ("\n");
    printf ("     SPECstorage(TM) Solution 2020 Release %24s\n", git_version);
    printf ("\n");
    printf ("\n");
    printf
	("     -----------------------------------------------------------------------\n");
    printf ("     This program contains contributions from Iozone.org.\n");
    printf ("\n");
    printf ("     Copyright (C) 2002-2020, Don Capps\n");
    printf ("     All rights reserved.\n");
    printf ("\n");
    printf
	("     This program contains a 64-bit version of Mersenne Twister pseudorandom \n");
    printf ("     number generator.\n");
    printf ("\n");
    printf
	("     Copyright (C) 2004, Makoto Matsumoto and Takuji Nishimura\n");
    printf ("     All rights reserved.\n");
    printf ("\n");
    printf
	("     Redistribution and use in source and binary forms, with or without\n");
    printf
	("     modification, are permitted provided that the following conditions \n");
    printf ("     are met:\n");
    printf ("\n");
    printf
	("     1. Redistributions of source code must retain the above copyright\n");
    printf
	("        notices, this list of conditions and the following disclaimer.\n");
    printf
	("     2. Redistributions in binary form must reproduce the above copyright\n");
    printf
	("        notices, this list of conditions and the following disclaimer in \n");
    printf
	("        the documentation and/or other materials provided with the \n");
    printf ("        distribution.\n");
    printf
	("     3. The names of its contributors may not be used to endorse or \n");
    printf
	("        promote products derived from this software without specific prior \n");
    printf ("        written permission.\n");
    printf ("\n");
    printf
	("     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n");
    printf
	("     \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n");
    printf
	("     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n");
    printf
	("     A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT \n");
    printf
	("     OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, \n");
    printf
	("     SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT \n");
    printf
	("     LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, \n");
    printf
	("     DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\n");
    printf
	("     THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT \n");
    printf
	("     (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n");
    printf
	("     OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n");
    printf
	("     -----------------------------------------------------------------------\n");
    printf ("\n");
    fflush (stdout);
#endif

#if defined(LITE)
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC,
	      "     *************************************************************************\n");
    log_file (LOG_EXEC, "     |     %-15s Release         %24s        |\n",
	      testname, git_version);
    log_file (LOG_EXEC,
	      "    |                                    by                                 |\n");
    log_file (LOG_EXEC, "     |     Author:       %-50s  |\n", Author);
    log_file (LOG_EXEC, "     |     Maintainer:   %-50s  |\n", Maintainer);
    log_file (LOG_EXEC, "     |     Contributors: %-50s  |\n", Contributors);
    log_file (LOG_EXEC, "     |                   %-50s  |\n", Contributors2);
    log_file (LOG_EXEC, "     |                   %-50s  |\n", Contributors3);
    log_file (LOG_EXEC, "     |                   %-50s  |\n", Contributors4);
    log_file (LOG_EXEC, "     |                   %-50s  |\n", Contributors5);
    log_file (LOG_EXEC,
	      "     *************************************************************************\n");
    log_file (LOG_EXEC, "      %s %s\n\n", short_testname, git_version);
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC,
	      "     This program contains contributions from Iozone.org.\n");
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC, "     Copyright (C) 2002-2020, Don Capps\n");
    log_file (LOG_EXEC, "     All rights reserved.\n");
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC,
	      "     This program contains a 64-bit version of Mersenne Twister pseudorandom \n");
    log_file (LOG_EXEC, "     number generator.\n");
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC,
	      "     Copyright (C) 2004, Makoto Matsumoto and Takuji Nishimura\n");
    log_file (LOG_EXEC, "     All rights reserved.\n");
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC,
	      "     Redistribution and use in source and binary forms, with or without\n");
    log_file (LOG_EXEC,
	      "     modification, are permitted provided that the following conditions \n");
    log_file (LOG_EXEC, "     are met:\n");
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC,
	      "     1. Redistributions of source code must retain the above copyright\n");
    log_file (LOG_EXEC,
	      "        notices, this list of conditions and the following disclaimer.\n");
    log_file (LOG_EXEC,
	      "     2. Redistributions in binary form must reproduce the above copyright\n");
    log_file (LOG_EXEC,
	      "        notices, this list of conditions and the following disclaimer in \n");
    log_file (LOG_EXEC,
	      "        the documentation and/or other materials provided with the \n");
    log_file (LOG_EXEC, "        distribution.\n");
    log_file (LOG_EXEC,
	      "     3. The names of its contributors may not be used to endorse or \n");
    log_file (LOG_EXEC,
	      "        promote products derived from this software without specific prior \n");
    log_file (LOG_EXEC, "        written permission.\n");
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC,
	      "     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n");
    log_file (LOG_EXEC,
	      "     \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n");
    log_file (LOG_EXEC,
	      "     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n");
    log_file (LOG_EXEC,
	      "     A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT \n");
    log_file (LOG_EXEC,
	      "     OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, \n");
    log_file (LOG_EXEC,
	      "     SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT \n");
    log_file (LOG_EXEC,
	      "     LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, \n");
    log_file (LOG_EXEC,
	      "     DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\n");
    log_file (LOG_EXEC,
	      "     THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT \n");
    log_file (LOG_EXEC,
	      "     (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n");
    log_file (LOG_EXEC,
	      "     OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n");
    log_file (LOG_EXEC, "\n");

#endif
#if defined(SPEC_DIST)
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC, "     SPECstorage(TM) Solution 2020 Release %24s\n",
	      git_version);
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC,
	      "     This product contains benchmarks acquired from several sources who\n");
    log_file (LOG_EXEC,
	      "     understand and agree with SPEC's goal of creating fair and objective\n");
    log_file (LOG_EXEC, "     benchmarks to measure computer performance.\n");
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC,
	      "     This copyright notice is placed here only to protect SPEC in the\n");
    log_file (LOG_EXEC,
	      "     event the source is misused in any manner that is contrary to the\n");
    log_file (LOG_EXEC, "     spirit, the goals and the intent of SPEC.\n");
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC,
	      "     The source code is provided to the user or company under the license\n");
    log_file (LOG_EXEC,
	      "     agreement for the SPEC Benchmark Suite for this product.\n");
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC,
	      "     -----------------------------------------------------------------------\n");
    log_file (LOG_EXEC,
	      "     This program contains contributions from Iozone.org.\n");
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC, "     Copyright (C) 2002-2020, Don Capps\n");
    log_file (LOG_EXEC, "     All rights reserved.\n");
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC,
	      "     This program contains a 64-bit version of Mersenne Twister pseudorandom \n");
    log_file (LOG_EXEC, "     number generator.\n");
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC,
	      "     Copyright (C) 2004, Makoto Matsumoto and Takuji Nishimura\n");
    log_file (LOG_EXEC, "     All rights reserved.\n");
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC,
	      "     Redistribution and use in source and binary forms, with or without\n");
    log_file (LOG_EXEC,
	      "     modification, are permitted provided that the following conditions \n");
    log_file (LOG_EXEC, "     are met:\n");
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC,
	      "     1. Redistributions of source code must retain the above copyright\n");
    log_file (LOG_EXEC,
	      "        notices, this list of conditions and the following disclaimer.\n");
    log_file (LOG_EXEC,
	      "     2. Redistributions in binary form must reproduce the above copyright\n");
    log_file (LOG_EXEC,
	      "        notices, this list of conditions and the following disclaimer in \n");
    log_file (LOG_EXEC,
	      "        the documentation and/or other materials provided with the \n");
    log_file (LOG_EXEC, "        distribution.\n");
    log_file (LOG_EXEC,
	      "     3. The names of its contributors may not be used to endorse or \n");
    log_file (LOG_EXEC,
	      "        promote products derived from this software without specific prior \n");
    log_file (LOG_EXEC, "        written permission.\n");
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC,
	      "     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n");
    log_file (LOG_EXEC,
	      "     \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n");
    log_file (LOG_EXEC,
	      "     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n");
    log_file (LOG_EXEC,
	      "     A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT \n");
    log_file (LOG_EXEC,
	      "     OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, \n");
    log_file (LOG_EXEC,
	      "     SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT \n");
    log_file (LOG_EXEC,
	      "     LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, \n");
    log_file (LOG_EXEC,
	      "     DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\n");
    log_file (LOG_EXEC,
	      "     THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT \n");
    log_file (LOG_EXEC,
	      "     (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n");
    log_file (LOG_EXEC,
	      "     OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n");
    log_file (LOG_EXEC,
	      "    -----------------------------------------------------------------------\n");
    log_file (LOG_EXEC, "\n");

#endif
#if defined(PRO)
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC, "     SPECstorage(TM) Solution 2020 Release %24s\n",
	      git_version);
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC,
	      "     -----------------------------------------------------------------------\n");
    log_file (LOG_EXEC,
	      "     This program contains contributions from Iozone.org.\n");
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC, "     Copyright (C) 2002-2020, Don Capps\n");
    log_file (LOG_EXEC, "     All rights reserved.\n");
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC,
	      "     This program contains a 64-bit version of Mersenne Twister pseudorandom \n");
    log_file (LOG_EXEC, "     number generator.\n");
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC,
	      "     Copyright (C) 2004, Makoto Matsumoto and Takuji Nishimura\n");
    log_file (LOG_EXEC, "     All rights reserved.\n");
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC,
	      "     Redistribution and use in source and binary forms, with or without\n");
    log_file (LOG_EXEC,
	      "     modification, are permitted provided that the following conditions \n");
    log_file (LOG_EXEC, "     are met:\n");
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC,
	      "     1. Redistributions of source code must retain the above copyright\n");
    log_file (LOG_EXEC,
	      "        notices, this list of conditions and the following disclaimer.\n");
    log_file (LOG_EXEC,
	      "     2. Redistributions in binary form must reproduce the above copyright\n");
    log_file (LOG_EXEC,
	      "        notices, this list of conditions and the following disclaimer in \n");
    log_file (LOG_EXEC,
	      "        the documentation and/or other materials provided with the \n");
    log_file (LOG_EXEC, "        distribution.\n");
    log_file (LOG_EXEC,
	      "     3. The names of its contributors may not be used to endorse or \n");
    log_file (LOG_EXEC,
	      "        promote products derived from this software without specific prior \n");
    log_file (LOG_EXEC, "        written permission.\n");
    log_file (LOG_EXEC, "\n");
    log_file (LOG_EXEC,
	      "     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n");
    log_file (LOG_EXEC,
	      "     \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n");
    log_file (LOG_EXEC,
	      "     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n");
    log_file (LOG_EXEC,
	      "     A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT \n");
    log_file (LOG_EXEC,
	      "     OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, \n");
    log_file (LOG_EXEC,
	      "     SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT \n");
    log_file (LOG_EXEC,
	      "     LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, \n");
    log_file (LOG_EXEC,
	      "     DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\n");
    log_file (LOG_EXEC,
	      "     THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT \n");
    log_file (LOG_EXEC,
	      "     (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n");
    log_file (LOG_EXEC,
	      "     OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n");
    log_file (LOG_EXEC,
	      "     -----------------------------------------------------------------------\n");
    log_file (LOG_EXEC, "\n");

#endif
}
