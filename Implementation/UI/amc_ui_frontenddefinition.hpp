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


#ifndef __AMC_UI_FRONTENDDEFINITION
#define __AMC_UI_FRONTENDDEFINITION

#include "common_chrono.hpp"

#include "amc_ui_expression.hpp"

#include <memory>
#include <map>

namespace AMC {

	enum class eUIFrontendDefinitionAttributeType : uint32_t {
		atUnknown = 0,
		atString = 1,
		atNumber = 2,
		atInteger = 3,
		atBoolean = 4,
		atUUID = 5,
		atArray = 6,
		atObjet = 7
	};

	class CUIFrontendDefinitionAttribute {
	private:

		std::string m_sName;
		eUIFrontendDefinitionAttributeType m_AttributeType;

	public:

		CUIFrontendDefinitionAttribute(const std::string& sName, eUIFrontendDefinitionAttributeType attributeType);

		virtual ~CUIFrontendDefinitionAttribute();

		std::string getName();

		eUIFrontendDefinitionAttributeType getAttributeType();
	};

	typedef std::shared_ptr<CUIFrontendDefinitionAttribute> PUIFrontendDefinitionAttribute;


	class CUIFrontendDefinitionExpressionAttribute : public CUIFrontendDefinitionAttribute {
	private:
		CUIExpression m_ValueExpression;

	public: 

		CUIFrontendDefinitionExpressionAttribute(const std::string& sName, eUIFrontendDefinitionAttributeType attributeType, const CUIExpression& valueExpression);

		virtual ~CUIFrontendDefinitionExpressionAttribute();

	};


	class CUIFrontendDefinitionModuleStore {
	private:

		std::string m_sPath;
		std::string m_sUUID;

		std::map<std::string, PUIFrontendDefinitionAttribute> m_Attributes;

	public:
		CUIFrontendDefinitionModuleStore(const std::string& sModuleUUID, const std::string & sModulePath);

		virtual ~CUIFrontendDefinitionModuleStore();

		PUIFrontendDefinitionAttribute registerValue (const std::string& sName, eUIFrontendDefinitionAttributeType attributeType, const CUIExpression & valueExpression);

	};

	typedef std::shared_ptr<CUIFrontendDefinitionModuleStore> PUIFrontendDefinitionModuleStore;

	class CUIFrontendDefinition {
	private:

		std::map<std::string, PUIFrontendDefinitionModuleStore> m_ModuleStores;
		AMCCommon::PChrono m_pGlobalChrono;

	public:

		CUIFrontendDefinition (AMCCommon::PChrono pGlobalChrono);

		virtual ~CUIFrontendDefinition ();

		PUIFrontendDefinitionModuleStore registerModuleStore (const std::string& sModuleUUID, const std::string& sPath);

		AMCCommon::PChrono getGlobalChrono();

	};

	typedef std::shared_ptr<CUIFrontendDefinition> PUIFrontendDefinition;

}

#endif //__AMC_UI_FRONTENDDEFINITION

