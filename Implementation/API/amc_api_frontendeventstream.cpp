/*++

Copyright (C) 2026 Autodesk Inc.

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

Abstract: This is the class definition of CAPIFrontendEventStream.

*/

#define __AMCIMPL_API_CONSTANTS

#include "amc_api_frontendeventstream.hpp"
#include "amc_api_sessionhandler.hpp"
#include "amc_api_constants.hpp"
#include "amc_systemstate.hpp"
#include "amc_ui_handler.hpp"

#include "libmc_interfaceexception.hpp"
#include "common_utils.hpp"

using namespace AMC;

CAPIFrontendEventStream::CAPIFrontendEventStream(const std::string& sUUID, PAPIAuth pAuth, std::shared_ptr<CSystemState> pSystemState, std::shared_ptr<CAPISessionHandler> pSessionHandler)
	: CJSONEventStreamInstance(sUUID),
	m_pAuth(pAuth),
	m_pSystemState(pSystemState),
	m_pSessionHandler(pSessionHandler),
	m_LastSessionRefresh(std::chrono::steady_clock::now()),
	m_RevisionLog(AMC_UI_FRONTEND_REVISIONLOG_DEPTH, createInitialRevision())
{
	if (pAuth.get() == nullptr)
		throw ELibMCInterfaceException(LIBMC_ERROR_INVALIDPARAM);
	if (pSystemState.get() == nullptr)
		throw ELibMCInterfaceException(LIBMC_ERROR_INVALIDPARAM);
	if (pSessionHandler.get() == nullptr)
		throw ELibMCInterfaceException(LIBMC_ERROR_INVALIDPARAM);
}

CAPIFrontendEventStream::~CAPIFrontendEventStream()
{
}

uint64_t CAPIFrontendEventStream::createInitialRevision()
{
	// Event IDs of different streams fall into disjoint ranges with high probability, so that a
	// Last-Event-ID of another session never resumes from this log. IDs stay below 2^53 for JavaScript clients.
	std::string sRandomHex = AMCCommon::CUtils::calculateRandomSHA256String(1);
	uint64_t nEpoch = std::stoull(sRandomHex.substr(0, 5), nullptr, 16) + 1;
	return nEpoch << 32;
}

PAPIAuth CAPIFrontendEventStream::getAuth()
{
	std::lock_guard<std::mutex> lockGuard(m_Mutex);
	return m_pAuth;
}

bool CAPIFrontendEventStream::refreshSessionIfDue(const std::string& sSessionUUID)
{
	auto now = std::chrono::steady_clock::now();
	{
		std::lock_guard<std::mutex> lockGuard(m_Mutex);
		if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastSessionRefresh).count() < AMC_API_FRONTENDEVENTSTREAM_SESSIONREFRESHINTERVAL_MS)
			return true;
		m_LastSessionRefresh = now;
	}

	auto pSessionHandler = m_pSessionHandler.lock();
	if (pSessionHandler.get() == nullptr)
		return false;

	return pSessionHandler->refreshSessionActivity(sSessionUUID);
}

void CAPIFrontendEventStream::endStream()
{
	{
		std::lock_guard<std::mutex> lockGuard(m_Mutex);
		m_pAuth = nullptr;
	}

	CJSONEventStreamInstance::endStream();
}

std::string CAPIFrontendEventStream::waitForNextEvent(sJSONEventStreamCursor& cursor)
{
	if (cursor.m_bStarted) {
		cursor.m_nChangeCounter = waitForChange(cursor.m_nChangeCounter, AMC_API_FRONTENDEVENTSTREAM_BUILDINTERVAL_MS);
	}
	else {
		cursor.m_nChangeCounter = getChangeCounter();
		cursor.m_bStarted = true;
	}

	if (!isActive())
		return "";

	auto pAuth = getAuth();
	auto pSystemState = m_pSystemState.lock();
	if ((pAuth.get() == nullptr) || (pSystemState.get() == nullptr)) {
		endStream();
		return "";
	}

	if (!refreshSessionIfDue(pAuth->getSessionUUID())) {
		endStream();
		return "";
	}

	CJSONWriter statusWriter;
	pSystemState->uiHandler()->frontendWriteStatusToJSON(statusWriter, pAuth.get(), nullptr, nullptr);

	CUIFrontendSnapshot snapshot;
	snapshot.readFromFrontendJSON(statusWriter.getDocument());

	uint64_t nBaseRevision = cursor.m_nLastEventID;
	auto publishResult = m_RevisionLog.publish(snapshot, "*", nBaseRevision);

	if (publishResult.m_bPatchAvailable) {
		if (publishResult.m_nRevision == nBaseRevision)
			return "";

		CJSONWriter patchWriter;
		patchWriter.addInteger(AMC_API_KEY_FRONTEND_REVISION, (int64_t)publishResult.m_nRevision);
		patchWriter.addInteger(AMC_API_KEY_FRONTEND_BASE, (int64_t)nBaseRevision);

		CJSONWriterObject changedObject(patchWriter);
		publishResult.m_Patch.writeToJSON(patchWriter, changedObject);
		patchWriter.addObject(AMC_API_KEY_FRONTEND_CHANGED, changedObject);

		cursor.m_nLastEventID = publishResult.m_nRevision;
		return formatEvent(AMC_API_FRONTENDEVENTSTREAM_EVENT_PATCH, publishResult.m_nRevision, patchWriter.saveToString());
	}

	CJSONWriter snapshotWriter;
	snapshotWriter.addInteger(AMC_API_KEY_FRONTEND_REVISION, (int64_t)publishResult.m_nRevision);

	CJSONWriterObject storesObject(snapshotWriter);
	snapshot.writeToJSON(snapshotWriter, storesObject);
	snapshotWriter.addObject(AMC_API_KEY_FRONTEND_STORES, storesObject);

	cursor.m_nLastEventID = publishResult.m_nRevision;
	return formatEvent(AMC_API_FRONTENDEVENTSTREAM_EVENT_SNAPSHOT, publishResult.m_nRevision, snapshotWriter.saveToString());
}
