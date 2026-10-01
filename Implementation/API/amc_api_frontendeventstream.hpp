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

Abstract: This is the class declaration of CAPIFrontendEventStream.
The server-sent event stream of one session. It pushes the unfiltered frontend
status as a snapshot event and every later change as a patch event.

*/


#ifndef __AMC_API_FRONTENDEVENTSTREAM
#define __AMC_API_FRONTENDEVENTSTREAM

#include "amc_jsoneventstreaminstance.hpp"
#include "amc_ui_frontendsnapshot.hpp"
#include "amc_api_auth.hpp"

#include <chrono>
#include <memory>
#include <mutex>
#include <string>

#define AMC_API_FRONTENDEVENTSTREAM_BUILDINTERVAL_MS 100
#define AMC_API_FRONTENDEVENTSTREAM_SESSIONREFRESHINTERVAL_MS 30000

#define AMC_API_FRONTENDEVENTSTREAM_EVENT_SNAPSHOT "snapshot"
#define AMC_API_FRONTENDEVENTSTREAM_EVENT_PATCH "patch"

namespace AMC {

	class CSystemState;
	class CAPISessionHandler;

	class CAPIFrontendEventStream : public CJSONEventStreamInstance {
	private:

		std::mutex m_Mutex;
		PAPIAuth m_pAuth;
		std::weak_ptr<CSystemState> m_pSystemState;
		std::weak_ptr<CAPISessionHandler> m_pSessionHandler;
		std::chrono::steady_clock::time_point m_LastSessionRefresh;

		// Owned by the stream rather than the session, since polling clients publish scoped snapshots into the session's log.
		CUIFrontendRevisionLog m_RevisionLog;

		PAPIAuth getAuth();

		// Returns false if the session does not exist anymore.
		bool refreshSessionIfDue(const std::string& sSessionUUID);

		static uint64_t createInitialRevision();

	public:

		CAPIFrontendEventStream(const std::string& sUUID, PAPIAuth pAuth, std::shared_ptr<CSystemState> pSystemState, std::shared_ptr<CAPISessionHandler> pSessionHandler);

		virtual ~CAPIFrontendEventStream();

		void endStream() override;

		std::string waitForNextEvent(sJSONEventStreamCursor& cursor) override;

	};

	typedef std::shared_ptr<CAPIFrontendEventStream> PAPIFrontendEventStream;

}


#endif //__AMC_API_FRONTENDEVENTSTREAM
