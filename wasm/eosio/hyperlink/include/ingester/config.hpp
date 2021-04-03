#pragma once

//EVENT TTL HALF MORE OF NEXUS TTL TO PREVENT RE-ENTRANCY
#define SUB_EVENT_TTL    5400
#define SUB_CONFIG_TBL   "sub.config"_n
#define SUB_EVENT_TBL    "sub.events"_n
#define SUB_SIGNER_TBL   "sub.signers"_n
#define SUB_EXEC_TBL     "sub.execs"_n
#define SUB_SHA_IDX      "bysha"_n