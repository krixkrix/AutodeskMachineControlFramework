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

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL AUTODESK INC. BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Abstract: This is a stub class definition of CSessionMetricsHandler

*/

#include "libmcdata_sessionmetricshandler.hpp"
#include "libmcdata_interfaceexception.hpp"

#include "common_utils.hpp"
#include "common_chrono.hpp"

using namespace LibMCData::Impl;

/*************************************************************************************************************************
 Class definition of CSessionMetricsHandler 
**************************************************************************************************************************/

CSessionMetricsHandler::CSessionMetricsHandler(AMCData::PSQLHandler pSQLHandler)
	: m_pSQLHandler(pSQLHandler)
{
	if (pSQLHandler.get() == nullptr)
		throw ELibMCDataInterfaceException(LIBMCDATA_ERROR_INVALIDPARAM);

}

void CSessionMetricsHandler::AddFrontendMetrics(const std::string & sSessionUUID, const std::string & sLabel, const LibMCData_uint64 nIntervalStart, const LibMCData_uint64 nIntervalEnd, const LibMCData_uint32 nRequestCount, const LibMCData_double dSumDurationMS, const LibMCData_double dMinDurationMS, const LibMCData_double dMaxDurationMS, const LibMCData_double dSumSqDurationMS, const LibMCData_uint64 nPayloadSumBytes, const LibMCData_uint64 nPayloadMaxBytes, const LibMCData_double dServerBuildSumMS, const LibMCData_uint64 nAbsoluteTimeStamp)
{
	std::string sNormalizedSessionUUID = AMCCommon::CUtils::normalizeUUIDString(sSessionUUID);
	std::string sTimestamp = AMCCommon::CChrono::convertToISO8601TimeUTC(nAbsoluteTimeStamp);
	std::string sNewUUID = AMCCommon::CUtils::createUUID();

	std::lock_guard<std::mutex> lockGuard(m_Mutex);

	std::string sInsertQuery = "INSERT INTO session_metrics (uuid, sessionuuid, label, intervalstart, intervalend, requestcount, sumduration, minduration, maxduration, sumsqduration, payloadsum, payloadmax, serverbuildsum, active, timestamp) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 1, ?)";
	auto pInsertStatement = m_pSQLHandler->prepareStatement(sInsertQuery);
	pInsertStatement->setString(1, sNewUUID);
	pInsertStatement->setString(2, sNormalizedSessionUUID);
	pInsertStatement->setString(3, sLabel);
	pInsertStatement->setInt64(4, (int64_t)nIntervalStart);
	pInsertStatement->setInt64(5, (int64_t)nIntervalEnd);
	pInsertStatement->setInt64(6, (int64_t)nRequestCount);
	pInsertStatement->setDouble(7, dSumDurationMS);
	pInsertStatement->setDouble(8, dMinDurationMS);
	pInsertStatement->setDouble(9, dMaxDurationMS);
	pInsertStatement->setDouble(10, dSumSqDurationMS);
	pInsertStatement->setInt64(11, (int64_t)nPayloadSumBytes);
	pInsertStatement->setInt64(12, (int64_t)nPayloadMaxBytes);
	pInsertStatement->setDouble(13, dServerBuildSumMS);
	pInsertStatement->setString(14, sTimestamp);
	pInsertStatement->execute();
}

