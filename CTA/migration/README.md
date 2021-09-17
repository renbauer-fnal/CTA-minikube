CASTOR to CTA migration tools
-----------------------------

This document provides a high-level view of the CASTOR to CTA migration tools. For details please refer to the tools' specific command line arguments and/or configuration files.

The metadata migration from CASTOR to CTA involves two main parts:
1. Populate the CTA Oracle catalogue with all tape-related metadata, consolidating the content of the CASTOR Nameserver and VMGR Oracle databases.
2. Populate the namespace of the embedded EOS CTA instance with all files' and directories' metadata.

The tools to perform the metadata migration from CASTOR to CTA are as follows:

* `begin_vo_export_to_cta.sh`: initiates the export from CASTOR by blocking user access to CASTOR and importing the CASTOR directories passed as arguments. The script internally calls the following command:
    * `eos-import-dirs`: imports a CASTOR directory tree and injects it in the EOS namespace.
* `export_production_tapepool_to_cta.sh`: wrapper bash script to migrate all files belonging to a given tapepool, checking that they are not used in CASTOR and taking care of disabling them in CASTOR. This script supports a dry-run mode, where "dry" is to be intended for CASTOR only. The script internally calls the following commands:
    * `tapepool_castor_to_cta.py`: migrates all files belonging to a given tapepool to CTA. `ARCHIVED` tapes are not migrated, `READONLY` and `DISABLED` ones are. This tool creates an intermediate table for the files, consumed by `eos-import-files`.
    * `eos-import-dirs --delta`: imports any additional/missing directory that was not imported by the first round of `eos-import-dirs`.
    * `eos-import-files`: reads all file-related metadata from the previously created intermediate table and injects it to the EOS namespace.
    * `complete_cta_export.py`: terminates an ongoing export, cleaning up the previously created intermediate tables and flagging all files as 'on CTA' and tapes as `EXPORTED`.
* `zerolen_castor_to_cta.py`: similarly to `tapepool_castor_to_cta.py`, migrates all 0-byte files belonging to a given VO to CTA. The criteria to infer whether a 0-byte file belongs to a VO is purely path-based: all and only files whose path starts with `/castor/cern.ch/<VO>` or `/castor/cern.ch/grid/<VO>` are taken into account. If the specified VO is `'ALL'`, then any non-migrated  0-byte files are taken into account.

## Procedure

The tools are designed to work as follows, for any given VO:

0. Make sure all tools are installed and operational in the **VO's CASTOR head nodes**. The tools require access to the CASTOR Nameserver and Stager databases, as well as to the CTA catalogue and the VO's CTA EOS instance. In particular, a `/etc/castor/CTACONFIG` file is required, with a similar format as the `/etc/castor/NSCONFIG` file, and a `/etc/cta/castor-migration.conf` file is required with the options detailed in the corresponding `.example` file.

1. Initiate the export from CASTOR by running the `begin_vo_export_to_cta.sh` script. Example:
    * `bash begin_vo_export_to_cta.sh --doit /atlas /grid/atlas`

   This script blocks user access to the given top-level directories in CASTOR.

   Alternatively, for a test import without blocking access to CASTOR, import all relevant directories one by one using `eos-import-dirs`. Example:
    * `eos-import-dirs /atlas`
    * `eos-import-dirs /grid/atlas`

> [warning] The list of "relevant" directories for a given VO is to be provided by the operator. No automatic heuristic is provided for this step.

2. For each VO's tapepool: perform the tapepool export using `export_production_tapepool_to_cta.sh`. Example:
    * `bash export_production_tapepool_to_cta.sh r_atlas_raw atlas eosctaatlas --doit`
    * `bash export_production_tapepool_to_cta.sh r_atlas_user atlas eosctaatlas --doit`

   This script looks for ongoing tape migrations in CASTOR and stops if it finds any.

> [warning] Please note that the script does **NOT** check against the CASTOR Repack instance, therefore the operator **must ensure** that there's no submitted or ongoing repack for the targeted tapepool!

> [warning] In order to efficiently import tape pools holding dual tape copies, and avoid a double pass over the EOS metadata import, such tape pools are imported in a single operation: assuming that a tape pool holding the 1st copies is requested to be exported, the corresponding tape pool holding the 2nd copies is **also exported**. Furthermore, to protect CTA, `tapepool_castor_to_cta.py` will abort if some files are found having their 1st tape copy and other files having their 2nd tape copy, all in the given tape pool. The process will also abort if 2nd copies exist but a single tape pool holding them cannot be identified (e.g. because there are two, or it was not found).

3. At the end of all VO's tapepool, perform the 0-byte files migration. Example:
    * `zerolen_castor_to_cta.py --vo atlas --instance eosctaatlas --doit`
    * `eos-import-dirs --delta /    # if needed`
    * `eos-import-files`
    * `complete_cta_export.py`

4. After all VOs have been successfully exported, a final run of step 3. is required in order to export the remaining 0-byte files from the CASTOR Namespace:
    * `zerolen_castor_to_cta.py --vo ALL --instance eosctapublic --doit`
    * `eos-import-dirs --delta /    # if needed`
    * `eos-import-files`
    * `complete_cta_export.py`

## Further Notes

The database migration phases are based on either idempotent operations or single transactions, in order to minimize the disruption in case of failures. In particular, `fileclass` and `tapepool` entities are imported into the `STORAGE_CLASS` and `TAPE_POOL` CTA tables via idempotent statements, potentially overwriting what is pre-existing in the CTA catalogue. Similarly, `LOGICAL_LIBRARY` entities are populated on the very first import, but never updated again. On the contrary, `ARCHIVE_FILE` and `TAPE_FILE` entities are inserted in bulk in a single non-idempotent transaction.

Nevertheless, in case of errors, the tools abort and the operator is expected to fix the issue and rerun the export, possibly re-executing by hand one of the commands documented above. Errors are accumulated in suitable Oracle tables both for the database migration and the EOS namespace injection tools. For the latter, please refer to their specific instructions.

In addition, the following tools are provided, which can be used as part of a restore/recovery procedure. Such procedure has deliberately **not** been automated and will have to be dealt with on a case by case basis.

* `vmgr_reenable_tapepool.sh`: reverts in CASTOR the export of a tapepool, which had been successfully exported to CTA. All related tapes are marked `FULL`.
* `cta-catalogue-remove-castor-tapes.py`: removes in the CTA catalogue all metadata related to CASTOR tapes for a given tapepool. If additional CTA tapes were added to the tapepool, they are left in the catalogue. If the tapepool concerns dual tape copies, metadata related to the other copies is **also removed**.

