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


#include "amc_statesignalhandler.hpp"

#define _STATESIGNAL_HEADERPROTECTION 

#include "amc_statesignal.hpp"

#include "libmc_exceptiontypes.hpp"

#include "common_utils.hpp"

namespace AMC {
	
	
	CStateSignalHandler::CStateSignalHandler()
	{
	}
	

	CStateSignalHandler::~CStateSignalHandler()
	{
	}
	
	void CStateSignalHandler::addSignalDefinition(const std::string& sInstanceName, const std::string& sSignalName, const std::list<CStateSignalParameter>& Parameters, const std::list<CStateSignalParameter>& Results, uint32_t nSignalReactionTimeOutInMS, uint32_t nSignalQueueSize)
	{

		if (sSignalName.length() == 0)
			throw ELibMCCustomException(LIBMC_ERROR_INVALIDSIGNALNAME, sInstanceName);


		auto iter = m_SignalMap.find(std::make_pair (sInstanceName, sSignalName));
		if (iter != m_SignalMap.end())
			throw ELibMCCustomException(LIBMC_ERROR_DUPLICATESIGNAL, sInstanceName + "/" + sSignalName);

		auto pSignal = std::make_shared<CStateSignalSlot>(sInstanceName, sSignalName, Parameters, Results, nSignalReactionTimeOutInMS, nSignalQueueSize);
		m_SignalMap.insert(std::make_pair(std::make_pair (sInstanceName, sSignalName), pSignal));
	}

	bool CStateSignalHandler::addNewInQueueSignal(const std::string& sInstanceName, const std::string& sSignalName, const std::string& sSignalUUID, const std::string& sParameterData, uint32_t nResponseTimeOutInMS)
	{

		auto iUUIDIter = m_SignalUUIDLookupMap.find(sSignalUUID);
		if (iUUIDIter != m_SignalUUIDLookupMap.end())
			throw ELibMCCustomException(LIBMC_ERROR_SIGNALALREADYTRIGGERED, sSignalUUID);

		auto iter = m_SignalMap.find(std::make_pair(sInstanceName, sSignalName));
		if (iter == m_SignalMap.end())
			throw ELibMCCustomException(LIBMC_ERROR_SIGNALNOTFOUND, sInstanceName + "/" + sSignalName);

		if (iter->second->addNewInQueueSignalInternal (sSignalUUID, sParameterData, nResponseTimeOutInMS)) {
			m_SignalUUIDLookupMap.insert(std::make_pair(sSignalUUID, iter->second));
			return true;
		}

		return false;
	}


	bool CStateSignalHandler::hasSignalDefinition(const std::string& sInstanceName, const std::string& sSignalName)
	{
		auto iter = m_SignalMap.find(std::make_pair(sInstanceName, sSignalName));
		return (iter != m_SignalMap.end());
	}



	void CStateSignalHandler::clearUnhandledSignals(const std::string& sInstanceName)
	{
		for (auto it = m_SignalMap.begin(); it != m_SignalMap.end(); it++) {
			// Check if the first element of the key matches
			if (it->first.first == sInstanceName) {
				it->second->clearQueueInternal();
			}
		}
	}

	void CStateSignalHandler::clearUnhandledSignalsOfType(const std::string& sInstanceName, const std::string& sSignalTypeName)
	{

		for (auto it = m_SignalMap.begin(); it != m_SignalMap.end(); it++) {
			// Check if the first element of the key matches
			if (it->first.first == sInstanceName) {
				if (it->second->getNameInternal() == sSignalTypeName) {
					it->second->clearQueueInternal();
				}
			}
		}

	}


	bool CStateSignalHandler::canTrigger(const std::string& sInstanceName, const std::string& sSignalName)
	{

		auto iter = m_SignalMap.find(std::make_pair(sInstanceName, sSignalName));
		if (iter == m_SignalMap.end())
			throw ELibMCCustomException(LIBMC_ERROR_SIGNALNOTFOUND, sInstanceName + "/" + sSignalName);

		return !iter->second->queueIsFull();
	}


	void CStateSignalHandler::changeSignalPhaseToHandled(const std::string& sSignalUUID, const std::string& sResultData)
	{

		auto iter = m_SignalUUIDLookupMap.find(sSignalUUID);
		if (iter == m_SignalUUIDLookupMap.end())
			throw ELibMCCustomException(LIBMC_ERROR_SIGNALNOTFOUND, sSignalUUID);

		iter->second->changeSignalPhaseToHandledInternal(sSignalUUID, sResultData);
	}

	void CStateSignalHandler::changeSignalPhaseToInProcess(const std::string& sSignalUUID)
	{

		auto iter = m_SignalUUIDLookupMap.find(sSignalUUID);
		if (iter == m_SignalUUIDLookupMap.end())
			throw ELibMCCustomException(LIBMC_ERROR_SIGNALNOTFOUND, sSignalUUID);

		iter->second->changeSignalPhaseToInProcessInternal (sSignalUUID);
	}

	void CStateSignalHandler::changeSignalPhaseToFailed(const std::string& sSignalUUID, const std::string& sResultData, const std::string& sErrorMessage)
	{

		auto iter = m_SignalUUIDLookupMap.find(sSignalUUID);
		if (iter == m_SignalUUIDLookupMap.end())
			throw ELibMCCustomException(LIBMC_ERROR_SIGNALNOTFOUND, sSignalUUID);

		iter->second->changeSignalPhaseToInFailedInternal(sSignalUUID, sResultData, sErrorMessage);
	}

	AMC::eAMCSignalPhase CStateSignalHandler::getSignalPhase(const std::string& sSignalUUID)
	{

		auto iter = m_SignalUUIDLookupMap.find(sSignalUUID);
		if (iter == m_SignalUUIDLookupMap.end())
			throw ELibMCCustomException(LIBMC_ERROR_SIGNALNOTFOUND, sSignalUUID);

		return iter->second->getSignalPhaseInternal (sSignalUUID);
	}

	std::string CStateSignalHandler::peekSignalMessageFromQueue(const std::string& sInstanceName, const std::string& sSignalName)
	{
		auto iter = m_SignalMap.find(std::make_pair(sInstanceName, sSignalName));
		if (iter == m_SignalMap.end())
			throw ELibMCCustomException(LIBMC_ERROR_SIGNALNOTFOUND, sInstanceName + "/" + sSignalName);

		return iter->second->peekMessageFromQueueInternal();

	}


	uint32_t CStateSignalHandler::getAvailableSignalQueueEntryCount(const std::string& sInstanceName, const std::string& sSignalName)
	{

		auto iter = m_SignalMap.find(std::make_pair(sInstanceName, sSignalName));
		if (iter == m_SignalMap.end())
			throw ELibMCCustomException(LIBMC_ERROR_SIGNALNOTFOUND, sInstanceName + "/" + sSignalName);
		
		return iter->second->getAvailableSignalQueueEntriesInternal ();

	}

	uint32_t CStateSignalHandler::getTotalSignalQueueSize(const std::string& sInstanceName, const std::string& sSignalName)
	{

		auto iter = m_SignalMap.find(std::make_pair(sInstanceName, sSignalName));
		if (iter == m_SignalMap.end())
			throw ELibMCCustomException(LIBMC_ERROR_SIGNALNOTFOUND, sInstanceName + "/" + sSignalName);

		return iter->second->getTotalSignalQueueSizeInternal();

	}

	uint32_t CStateSignalHandler::getDefaultReactionTimeout(const std::string& sInstanceName, const std::string& sSignalName)
	{

		auto iter = m_SignalMap.find(std::make_pair(sInstanceName, sSignalName));
		if (iter == m_SignalMap.end())
			throw ELibMCCustomException(LIBMC_ERROR_SIGNALNOTFOUND, sInstanceName + "/" + sSignalName);

		return iter->second->getDefaultReactionTimeoutInternal();


	}

	uint32_t CStateSignalHandler::getReactionTimeout(const std::string& sSignalUUID)
	{

		auto iter = m_SignalUUIDLookupMap.find(sSignalUUID);
		if (iter == m_SignalUUIDLookupMap.end())
			throw ELibMCCustomException(LIBMC_ERROR_SIGNALNOTFOUND, sSignalUUID);

		return iter->second->getReactionTimeoutInternal(sSignalUUID);

	}

	std::string CStateSignalHandler::getResultDataJSON(const std::string& sSignalUUID)
	{

		auto iter = m_SignalUUIDLookupMap.find(sSignalUUID);
		if (iter == m_SignalUUIDLookupMap.end())
			throw ELibMCCustomException(LIBMC_ERROR_SIGNALNOTFOUND, sSignalUUID);

		return iter->second->getResultDataJSONInternal(sSignalUUID);

	}

	bool CStateSignalHandler::findSignalPropertiesByUUID(const std::string& sSignalUUID, std::string& sInstanceName, std::string& sSignalName, std::string& sParameterData)
	{

		auto iter = m_SignalUUIDLookupMap.find(sSignalUUID);
		if (iter != m_SignalUUIDLookupMap.end()) {
			sInstanceName = iter->second->getInstanceNameInternal();
			sSignalName = iter->second->getNameInternal();
			sParameterData = iter->second->getParameterDataJSONInternal(sSignalUUID);
			return true;		
		}

		return false;

	}


	void CStateSignalHandler::populateParameterGroup(const std::string& sInstanceName, const std::string& sSignalName, CParameterGroup* pParameterGroup)
	{

		LibMCAssertNotNull(pParameterGroup);

		auto iter = m_SignalMap.find(std::make_pair(sInstanceName, sSignalName));
		if (iter == m_SignalMap.end())
			throw ELibMCCustomException(LIBMC_ERROR_SIGNALNOTFOUND, sInstanceName + "/" + sSignalName);

		iter->second->populateParameterGroup(pParameterGroup);
	}

	void CStateSignalHandler::populateResultGroup(const std::string& sInstanceName, const std::string& sSignalName, CParameterGroup* pResultGroup)
	{

		LibMCAssertNotNull(pResultGroup);

		auto iter = m_SignalMap.find(std::make_pair(sInstanceName, sSignalName));
		if (iter == m_SignalMap.end())
			throw ELibMCCustomException(LIBMC_ERROR_SIGNALNOTFOUND, sInstanceName + "/" + sSignalName);

		iter->second->populateResultGroup(pResultGroup);
	}



}


