/*++

Copyright (C) 2020 Autodesk Inc.

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
	* Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.
	* Neither the name of the Autodesk Inc. nor the
	  names of its contributors may be used to endorse or promote products
	  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL AUTODESK INC. BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#include "amcdata_databasemigrator_sessionmetrics.hpp"
#include "libmcdata_interfaceexception.hpp"

namespace AMCData {
		
	void CDatabaseMigrationClass_SessionMetrics::increaseSchemaVersion(PSQLTransaction pTransaction, uint32_t nCurrentVersionIndex)
	{

		if (pTransaction.get() == nullptr)
			throw ELibMCDataInterfaceException(LIBMCDATA_ERROR_INVALIDPARAM);

		switch (nCurrentVersionIndex) {
		case 23: {
			// Per-session client reactivity metrics. Each row aggregates the
			// client-side roundtrip measurements (and folded-in server build time
			// and payload sizes) of the frontend polling requests over a short
			// measurement window. The moments (count, sum, min, max, sumsq) are
			// stored raw so that mean/stddev can be derived and rows re-aggregated.
			std::string sSessionMetrics = "CREATE TABLE `session_metrics` (";
			sSessionMetrics += "`uuid` varchar ( 64 ) UNIQUE NOT NULL,";
			sSessionMetrics += "`sessionuuid` varchar ( 64 ) NOT NULL,";
			sSessionMetrics += "`label` varchar ( 256 ) NOT NULL,";
			sSessionMetrics += "`intervalstart` integer NOT NULL,";
			sSessionMetrics += "`intervalend` integer NOT NULL,";
			sSessionMetrics += "`requestcount` integer NOT NULL,";
			sSessionMetrics += "`sumduration` double NOT NULL,";
			sSessionMetrics += "`minduration` double NOT NULL,";
			sSessionMetrics += "`maxduration` double NOT NULL,";
			sSessionMetrics += "`sumsqduration` double NOT NULL,";
			sSessionMetrics += "`payloadsum` integer NOT NULL,";
			sSessionMetrics += "`payloadmax` integer NOT NULL,";
			sSessionMetrics += "`serverbuildsum` double NOT NULL,";
			sSessionMetrics += "`active` integer DEFAULT 1,";
			sSessionMetrics += "`timestamp` varchar ( 64 ) NOT NULL)";
			pTransaction->executeStatement(sSessionMetrics);

			std::string sSessionMetricsIndex = "CREATE INDEX `idx_session_metrics_session` ";
			sSessionMetricsIndex += "ON `session_metrics` (`sessionuuid`)";
			pTransaction->executeStatement(sSessionMetricsIndex);

			break;
		}

		}
	}



}

