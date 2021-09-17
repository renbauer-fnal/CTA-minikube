mapping files for tape-verify containing for each VID
      # format of map file:   <fseq1>  <wrap ID> <LPOS> <block id (dec)> separated by (one or more) spaces
      #                       <fseq2>  <wrap ID> <LPOS> <block id (dec)>
      #                       ....
      #                       EOD <wrap ID> <LPOS> <block id (dec)> -> this EOD entry is for signaling the EOD information (end of last segment)


generated with

for i in `grep VID /afs/cern.ch/project/castor/tape/tests/LTO/LTO-8/check-each-file-position-LTO-8.log|cut -d , -f 1|awk '{print $2}'|sort|uniq`; do echo -- $i --- ; grep $i /afs/cern.ch/project/castor/tape/tests/LTO/LTO-8/check-each-file-position-LTO-8.log|sed -e 's/,//g'|awk '{if ($NF == "EOD") {print $NF " " $6 " " $9 " " $13} else {print $4 " " $6 " " $9 " " $13}}' > /afs/cern.ch/project/castor/tape/tests/LTO/physmapfiles/$i.pmap; done


