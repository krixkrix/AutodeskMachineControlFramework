/*++

Copyright (C) 2025 Autodesk Inc.

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

#include "amc_api_handler_frontend.hpp"
#include "amc_api_jsonrequest.hpp"
#include "amc_ui_handler.hpp"
#include "amc_ui_module_item.hpp"

#define __AMCIMPL_UI_DIALOG
#define __AMCIMPL_UI_PAGE
#define __AMCIMPL_UI_MODULE

#include "amc_ui_page.hpp"
#include "amc_ui_dialog.hpp"
#include "amc_ui_module.hpp"
#include "amc_ui_clientaction.hpp"
#include "amc_meshentity.hpp"
#include "amc_meshhandler.hpp"
#include "amc_dataserieshandler.hpp"
#include "amc_scatterplot.hpp"
#include "amc_toolpathhandler.hpp"

#include "libmc_interfaceexception.hpp"
#include "libmcdata_dynamic.hpp"

#include "common_utils.hpp"
#include "common_chrono.hpp"

#include <cmath>
#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <set>
#include <sstream>
#include <iostream>


using namespace AMC;



CAPIHandler_Frontend::CAPIHandler_Frontend(PSystemState pSystemState)
	: CAPIHandler(pSystemState->getClientHash()), m_pSystemState(pSystemState)
{
	if (pSystemState.get() == nullptr)
		throw ELibMCInterfaceException(LIBMC_ERROR_INVALIDPARAM);


}

CAPIHandler_Frontend::~CAPIHandler_Frontend()
{

}

std::string CAPIHandler_Frontend::getBaseURI()
{
	return "api/frontend";
}

APIHandler_FrontendType CAPIHandler_Frontend::parseRequest(const std::string& sURI, const eAPIRequestType requestType, std::string& sParameterUUID, std::string& sAdditionalParameter)
{
	// Leave away base URI
	auto sParameterString = AMCCommon::CUtils::toLowerString(sURI.substr(getBaseURI().length()));
	sParameterUUID = "";
	sAdditionalParameter = "";

	if (requestType == eAPIRequestType::rtGet) {

		if (sParameterString.empty () || (sParameterString == "/"))
			return APIHandler_FrontendType::ftStatus;

	}


	if (requestType == eAPIRequestType::rtPost) {

		if ((sParameterString == "/metrics") || (sParameterString == "/metrics/"))
			return APIHandler_FrontendType::ftMetrics;

	}

	return APIHandler_FrontendType::ftUnknown;
}


void CAPIHandler_Frontend::checkAuthorizationMode(const std::string& sURI, const eAPIRequestType requestType, bool& bNeedsToBeAuthorized, bool& bCreateNewSession)
{
	bNeedsToBeAuthorized = true; 
	bCreateNewSession = false;
	
}

bool CAPIHandler_Frontend::expectsRawBody(const std::string& sURI, const eAPIRequestType requestType)
{
	std::string sParameterUUID;
	std::string sAdditionalParameter;
	auto uiType = parseRequest(sURI, requestType, sParameterUUID, sAdditionalParameter);

	return (uiType == APIHandler_FrontendType::ftTriggerEvent) || (uiType == APIHandler_FrontendType::ftMetrics);

}

void CAPIHandler_Frontend::handleStatusRequest(CJSONWriter& writer, CAPIFormFields& pFormFields, PAPIAuth pAuth)
{
	if (pAuth.get() == nullptr)
		throw ELibMCInterfaceException(LIBMC_ERROR_INVALIDPARAM);

	// Measure how long the server takes to build the status payload so the client can
	// fold this into its aggregated reactivity metrics (see handleMetricsRequest).
	auto pGlobalChrono = m_pSystemState->globalChrono();
	uint64_t nBuildStart = pGlobalChrono->getUTCTimeStampInMicrosecondsSince1970();

	auto splitNames = [](const std::string& sValue) -> std::set<std::string> {
		std::set<std::string> names;
		std::stringstream stream(sValue);
		std::string sName;
		while (std::getline(stream, sName, ',')) {
			if (!sName.empty())
				names.insert(sName);
		}
		return names;
	};

	if (pFormFields.hasRequestParameter(AMC_API_KEY_FRONTEND_ACTIVEPAGES)) {
		auto activePageNames = splitNames(pFormFields.getRequestParameter(AMC_API_KEY_FRONTEND_ACTIVEPAGES, false));
		auto activeDialogNames = splitNames(pFormFields.getRequestParameter(AMC_API_KEY_FRONTEND_ACTIVEDIALOGS, false));
		m_pSystemState->uiHandler()->frontendWriteStatusToJSON(writer, pAuth.get(), &activePageNames, &activeDialogNames);
	}
	else {
		m_pSystemState->uiHandler()->frontendWriteStatusToJSON(writer, pAuth.get(), nullptr, nullptr);
	}

	uint64_t nBuildEnd = pGlobalChrono->getUTCTimeStampInMicrosecondsSince1970();
	double dBuildTimeMS = (double)(nBuildEnd - nBuildStart) / 1000.0;
	writer.addDouble(AMC_API_KEY_FRONTEND_SERVERTIME, dBuildTimeMS);
}


void CAPIHandler_Frontend::handleMetricsRequest(CJSONWriter& writer, const uint8_t* pBodyData, const size_t nBodyDataSize, PAPIAuth pAuth)
{
	if (pAuth.get() == nullptr)
		throw ELibMCInterfaceException(LIBMC_ERROR_INVALIDPARAM);
	if (pBodyData == nullptr)
		throw ELibMCInterfaceException(LIBMC_ERROR_INVALIDPARAM);

	// The session identity is taken from the authenticated session and never trusted from the request body.
	std::string sSessionUUID = pAuth->getSessionUUID();

	CAPIJSONRequest jsonRequest(pBodyData, nBodyDataSize);

	std::string sLabel = "frontend";
	if (jsonRequest.hasValue(AMC_API_KEY_FRONTEND_METRICS_LABEL))
		sLabel = jsonRequest.getNameString(AMC_API_KEY_FRONTEND_METRICS_LABEL, LIBMC_ERROR_INVALIDPARAM);

	uint64_t nIntervalStart = jsonRequest.getUint64(AMC_API_KEY_FRONTEND_METRICS_INTERVALSTART, 0, UINT64_MAX, LIBMC_ERROR_INVALIDPARAM);
	uint64_t nIntervalEnd = jsonRequest.getUint64(AMC_API_KEY_FRONTEND_METRICS_INTERVALEND, 0, UINT64_MAX, LIBMC_ERROR_INVALIDPARAM);
	uint64_t nRequestCount = jsonRequest.getUint64(AMC_API_KEY_FRONTEND_METRICS_REQUESTCOUNT, 0, UINT32_MAX, LIBMC_ERROR_INVALIDPARAM);
	double dSumDurationMS = jsonRequest.getDouble(AMC_API_KEY_FRONTEND_METRICS_SUMDURATION, LIBMC_ERROR_INVALIDPARAM);
	double dMinDurationMS = jsonRequest.getDouble(AMC_API_KEY_FRONTEND_METRICS_MINDURATION, LIBMC_ERROR_INVALIDPARAM);
	double dMaxDurationMS = jsonRequest.getDouble(AMC_API_KEY_FRONTEND_METRICS_MAXDURATION, LIBMC_ERROR_INVALIDPARAM);
	double dSumSqDurationMS = jsonRequest.getDouble(AMC_API_KEY_FRONTEND_METRICS_SUMSQDURATION, LIBMC_ERROR_INVALIDPARAM);

	uint64_t nPayloadSumBytes = 0;
	if (jsonRequest.hasValue(AMC_API_KEY_FRONTEND_METRICS_PAYLOADSUM))
		nPayloadSumBytes = jsonRequest.getUint64(AMC_API_KEY_FRONTEND_METRICS_PAYLOADSUM, 0, UINT64_MAX, LIBMC_ERROR_INVALIDPARAM);
	uint64_t nPayloadMaxBytes = 0;
	if (jsonRequest.hasValue(AMC_API_KEY_FRONTEND_METRICS_PAYLOADMAX))
		nPayloadMaxBytes = jsonRequest.getUint64(AMC_API_KEY_FRONTEND_METRICS_PAYLOADMAX, 0, UINT64_MAX, LIBMC_ERROR_INVALIDPARAM);
	double dServerBuildSumMS = 0.0;
	if (jsonRequest.hasValue(AMC_API_KEY_FRONTEND_METRICS_SERVERBUILDSUM))
		dServerBuildSumMS = jsonRequest.getDouble(AMC_API_KEY_FRONTEND_METRICS_SERVERBUILDSUM, LIBMC_ERROR_INVALIDPARAM);

	auto pGlobalChrono = m_pSystemState->globalChrono();
	auto pDataModel = m_pSystemState->getDataModelInstance();
	auto pMetricsHandler = pDataModel->CreateSessionMetricsHandler();

	pMetricsHandler->AddFrontendMetrics(sSessionUUID, sLabel, nIntervalStart, nIntervalEnd, (LibMCData_uint32)nRequestCount, dSumDurationMS, dMinDurationMS, dMaxDurationMS, dSumSqDurationMS, nPayloadSumBytes, nPayloadMaxBytes, dServerBuildSumMS, pGlobalChrono->getUTCTimeStampInMicrosecondsSince1970());

	writer.addBoolean(AMC_API_KEY_FRONTEND_METRICS_RECORDED, true);
}


PAPIResponse CAPIHandler_Frontend::handleRequest(const std::string& sURI, const eAPIRequestType requestType, CAPIFormFields & pFormFields, const uint8_t* pBodyData, const size_t nBodyDataSize, PAPIAuth pAuth)
{
	std::string sParameterUUID;
	std::string sAdditionalParameter;
	auto uiType = parseRequest(sURI, requestType, sParameterUUID, sAdditionalParameter);

	CJSONWriter writer;
	writeJSONHeader(writer, AMC_API_PROTOCOL_FRONTEND);

	switch (uiType) {
	case APIHandler_FrontendType::ftStatus:
		handleStatusRequest(writer, pFormFields, pAuth);
		break;

	case APIHandler_FrontendType::ftMetrics:
		handleMetricsRequest(writer, pBodyData, nBodyDataSize, pAuth);
		break;

	default:
		throw ELibMCInterfaceException(LIBMC_ERROR_INVALIDPARAM);

	}

	return std::make_shared<CAPIStringResponse>(AMC_API_HTTP_SUCCESS, AMC_API_CONTENTTYPE, writer.saveToString());
}



		

