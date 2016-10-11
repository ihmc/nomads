Externals notes by Chris Eagle


CVS Reorganization brought to light serveral items that need to be
corrected or changed.  The following items need to be done:

1. JavaSNMP.jar is being replaced by snmp4-13.jar; however
   some files within the aci project are still pointing to
   JavaSNMP.jar.  ffaci depends on aci, and therefore by indirect
   relation ffaci is dependent on JavaSNMP.jar through aci.

-- I talked with Marco C. 18 Oct 2004, we will include both jars
   for the time being.  Marco will deprecate all files requiring
   the phased out jar at a later date.


2, Distribution for each project build currently copies entire
   externals directory, need to go back and copy only the required
   jar files.

3. kaos_arlada.jar is corrupted...looks as though it was add to
   cvs with "kv" vs "kb"
