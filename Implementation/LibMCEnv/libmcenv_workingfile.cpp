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


Abstract: This is a stub class definition of CWorkingFile

*/

#include "libmcenv_workingfile.hpp"
#include "libmcenv_interfaceexception.hpp"

// Include custom headers here.
#include "common_utils.hpp"
#include "common_importstream_native.hpp"
#include <cstring>

using namespace LibMCEnv::Impl;

#define WORKINGFILEBUFFER_MINIMUMSIZEINKB 1
#define WORKINGFILEBUFFER_MAXIMUMSIZEINKB (1024 * 1024)

CWorkingFileWriterInstance::CWorkingFileWriterInstance(const std::string& sLocalFileName, const std::string& sAbsoluteFileName, uint32_t nMemoryBufferSizeInKB)
    : m_sLocalFileName(sLocalFileName), m_sAbsoluteFileName(sAbsoluteFileName), m_nPositionInBuffer(0), m_nBytesWritten(0)
{
    if ((nMemoryBufferSizeInKB < WORKINGFILEBUFFER_MINIMUMSIZEINKB) ||
        (nMemoryBufferSizeInKB > WORKINGFILEBUFFER_MAXIMUMSIZEINKB)) {

        throw ELibMCEnvInterfaceException(LIBMCENV_ERROR_INVALIDWRITEBUFFERSIZE, std::to_string (nMemoryBufferSizeInKB));
    }

    m_MemoryBuffer.resize(nMemoryBufferSizeInKB * 1024);

    m_pExportStream = std::make_shared <AMCCommon::CExportStream_Native> (sAbsoluteFileName);

}

CWorkingFileWriterInstance::~CWorkingFileWriterInstance()
{
    finish();
}

std::string CWorkingFileWriterInstance::getAbsoluteFileName()
{
    return m_sAbsoluteFileName;
}

std::string CWorkingFileWriterInstance::getLocalFileName()
{
    return m_sLocalFileName;
}

void CWorkingFileWriterInstance::writeData(const uint8_t* pData, uint64_t nSize)
{
    uint64_t nBufferSize = m_MemoryBuffer.size();
        
    if (nSize == 0)
        return;

    if (pData == nullptr) 
        throw ELibMCEnvInterfaceException(LIBMCENV_ERROR_INVALIDPARAM);

    const uint8_t* pSource = pData;

    uint64_t nBytesLeft = nSize;
    while (nBytesLeft > 0) {
        if (m_nPositionInBuffer >= nBufferSize)
            throw ELibMCEnvInterfaceException(LIBMCENV_ERROR_INVALIDWRITEBUFFFERPOSITION, std::to_string(m_nPositionInBuffer));

        uint64_t nBytesAvailable = nBufferSize - m_nPositionInBuffer;
        uint64_t nBytesToCopy;

        if (nBytesLeft > nBytesAvailable)
            nBytesToCopy = nBytesAvailable;
        else
            nBytesToCopy = nBytesLeft;

        memcpy((void*)&m_MemoryBuffer.at(m_nPositionInBuffer), (void*)pSource, nBytesToCopy);
        pSource += nBytesToCopy;
        m_nPositionInBuffer += nBytesToCopy;

        if (m_nPositionInBuffer >= nBufferSize)
            flushBuffer();

        nBytesLeft -= nBytesToCopy;
    }
}

void CWorkingFileWriterInstance::flushBuffer()
{
    if (m_nPositionInBuffer > 0) {
        if (m_pExportStream.get() == nullptr)
            throw ELibMCEnvInterfaceException(LIBMCENV_ERROR_CANNOTWRITETOFINISHEDWORKINGFILE);

        m_pExportStream->writeBuffer(m_MemoryBuffer.data(), m_nPositionInBuffer);
    }

    m_nPositionInBuffer = 0;
}

void CWorkingFileWriterInstance::finish()
{
    flushBuffer();

    if (m_pExportStream.get() != nullptr) {
        m_pExportStream = nullptr;
    }
}

bool CWorkingFileWriterInstance::isFinished()
{
    return (m_pExportStream.get() == nullptr);
}


uint64_t CWorkingFileWriterInstance::getWrittenBytes()
{
    return m_nBytesWritten;
}


CWorkingFileMonitor::CWorkingFileMonitor(const std::string& sWorkingDirectory)
    : m_sWorkingDirectory (sWorkingDirectory), m_bIsActive (true)
{
    if (sWorkingDirectory.empty())
        throw ELibMCEnvInterfaceException(LIBMCENV_ERROR_INVALIDPARAM);
}


std::string CWorkingFileMonitor::getWorkingDirectory()
{
    return m_sWorkingDirectory;
}


std::string CWorkingFileMonitor::getAbsoluteFileName(const std::string& sFileName)
{
    if (!m_bIsActive)
        throw ELibMCEnvInterfaceException(LIBMCENV_ERROR_WORKINGDIRECTORYHASBEENCLEANED);

    if (sFileName.find(":") != std::string::npos)
        throw ELibMCEnvInterfaceException(LIBMCENV_ERROR_INVALIDCHARACTERINFILENAME);
    if (sFileName.find(">") != std::string::npos)
        throw ELibMCEnvInterfaceException(LIBMCENV_ERROR_INVALIDCHARACTERINFILENAME);
    if (sFileName.find("<") != std::string::npos)
        throw ELibMCEnvInterfaceException(LIBMCENV_ERROR_INVALIDCHARACTERINFILENAME);
    if (sFileName.find("\"") != std::string::npos)
        throw ELibMCEnvInterfaceException(LIBMCENV_ERROR_INVALIDCHARACTERINFILENAME);
    if (sFileName.find("/") != std::string::npos)
        throw ELibMCEnvInterfaceException(LIBMCENV_ERROR_INVALIDCHARACTERINFILENAME);
    if (sFileName.find("\\") != std::string::npos)
        throw ELibMCEnvInterfaceException(LIBMCENV_ERROR_INVALIDCHARACTERINFILENAME);
    if (sFileName.find("|") != std::string::npos)
        throw ELibMCEnvInterfaceException(LIBMCENV_ERROR_INVALIDCHARACTERINFILENAME);
    if (sFileName.find("?") != std::string::npos)
        throw ELibMCEnvInterfaceException(LIBMCENV_ERROR_INVALIDCHARACTERINFILENAME);
    if (sFileName.find("*") != std::string::npos)
        throw ELibMCEnvInterfaceException(LIBMCENV_ERROR_INVALIDCHARACTERINFILENAME);
    if (sFileName.find("..") != std::string::npos)
        throw ELibMCEnvInterfaceException(LIBMCENV_ERROR_INVALIDCHARACTERINFILENAME);

    return m_sWorkingDirectory + "/" + sFileName;
}

void CWorkingFileMonitor::addNewMonitoredFile(const std::string& sFileName)
{
    if (!m_bIsActive)
        throw ELibMCEnvInterfaceException(LIBMCENV_ERROR_WORKINGDIRECTORYHASBEENCLEANED);

    // Check if file name has the right character set.
    getAbsoluteFileName(sFileName);

    // Store file name
    m_MonitoredFileNames.insert(sFileName);
}

PWorkingFileWriterInstance CWorkingFileMonitor::addNewFileWriter(const std::string& sFileName, uint32_t nMemoryBufferSize)
{
    if (!m_bIsActive)
        throw ELibMCEnvInterfaceException(LIBMCENV_ERROR_WORKINGDIRECTORYHASBEENCLEANED);

    std::string sAbsoluteFileName = getAbsoluteFileName(sFileName);

    auto pInstance = std::make_shared <CWorkingFileWriterInstance> (sFileName, sAbsoluteFileName, nMemoryBufferSize);
    m_WriterInstances.insert (std::make_pair (sFileName, pInstance));

    addNewMonitoredFile(sFileName);

    return pInstance;
}


bool CWorkingFileMonitor::fileIsMonitored(const std::string& sFileName)
{
    if (!m_bIsActive)
        throw ELibMCEnvInterfaceException(LIBMCENV_ERROR_WORKINGDIRECTORYHASBEENCLEANED);

    auto iIter = m_MonitoredFileNames.find(sFileName);
    return (iIter != m_MonitoredFileNames.end());
}

void CWorkingFileMonitor::cleanUpDirectory(AMC::CLogger* pLoggerForUnmanagedFileWarnings)
{

    if (AMCCommon::CUtils::fileOrPathExistsOnDisk(m_sWorkingDirectory)) {
        for (auto sFileName : m_MonitoredFileNames) {
            auto sAbsoluteFileName = getAbsoluteFileName(sFileName);
            if (AMCCommon::CUtils::fileOrPathExistsOnDisk(sAbsoluteFileName)) {
                try
                {
                    AMCCommon::CUtils::deleteFileFromDisk(sAbsoluteFileName, true);
                }
                catch (std::exception& E)
                {
                    if (pLoggerForUnmanagedFileWarnings != nullptr)
                        pLoggerForUnmanagedFileWarnings->logMessage("Could not delete temporary file: " + sAbsoluteFileName + "(" + E.what() + ")", LOG_SUBSYSTEM_WORKINGDIRECTORIES, AMC::eLogLevel::Warning);
                }

            }
        }
    }


    try {
        if (AMCCommon::CUtils::fileOrPathExistsOnDisk(m_sWorkingDirectory)) {
            AMCCommon::CUtils::deleteDirectoryFromDisk(m_sWorkingDirectory, true);
        }
    }
    catch (std::exception& E)
    {
        if (pLoggerForUnmanagedFileWarnings != nullptr)
            pLoggerForUnmanagedFileWarnings->logMessage("Could not delete working directory: " + m_sWorkingDirectory + "(" + E.what() + ")", LOG_SUBSYSTEM_WORKINGDIRECTORIES, AMC::eLogLevel::Warning);
    }

    m_MonitoredFileNames.clear();
    m_bIsActive = false;

}

bool CWorkingFileMonitor::isActive()
{
    return m_bIsActive;
}


std::set<std::string> CWorkingFileMonitor::getFileNames()
{
    return m_MonitoredFileNames;
}

/*************************************************************************************************************************
 Class definition of CWorkingFile 
**************************************************************************************************************************/

CWorkingFile::CWorkingFile(const std::string & sFileName, PWorkingFileMonitor pWorkingFileMonitor)
    : m_pWorkingFileMonitor (pWorkingFileMonitor)
{
    if (pWorkingFileMonitor.get() == nullptr)
        throw ELibMCEnvInterfaceException(LIBMCENV_ERROR_INVALIDPARAM);

    m_sFileName = sFileName;
    m_sAbsolutePath = pWorkingFileMonitor->getAbsoluteFileName(sFileName);
}


CWorkingFile* CWorkingFile::makeFrom(CWorkingFile* pWorkingFile)
{
    if (pWorkingFile == nullptr)
        throw ELibMCEnvInterfaceException(LIBMCENV_ERROR_INVALIDPARAM);

    return new CWorkingFile (pWorkingFile->m_sFileName, pWorkingFile->m_pWorkingFileMonitor);
}


std::shared_ptr<CWorkingFile> CWorkingFile::makeSharedFrom(CWorkingFile* pWorkingFile)
{
    return std::shared_ptr <CWorkingFile> (makeFrom (pWorkingFile));
}

std::string CWorkingFile::GetAbsoluteFileName()
{
    return m_sAbsolutePath;
}

LibMCEnv_uint64 CWorkingFile::GetSize()
{
    if (!AMCCommon::CUtils::fileOrPathExistsOnDisk(m_sAbsolutePath))
        return 0;

    auto pStream = std::make_shared<AMCCommon::CImportStream_Native> (m_sAbsolutePath);
    return pStream->retrieveSize();
}

void CWorkingFile::ReadContent(LibMCEnv_uint64 nFileContentBufferSize, LibMCEnv_uint64* pFileContentNeededCount, LibMCEnv_uint8* pFileContentBuffer)
{
    if (!AMCCommon::CUtils::fileOrPathExistsOnDisk(m_sAbsolutePath))
        throw ELibMCEnvInterfaceException(LIBMCENV_ERROR_WORKINGFILEDOESNOTEXIST);

    auto pStream = std::make_shared<AMCCommon::CImportStream_Native>(m_sAbsolutePath);
    uint64_t nFileSize = pStream->retrieveSize();

	if (pFileContentNeededCount != nullptr)
		*pFileContentNeededCount = nFileSize;

	if (pFileContentBuffer != nullptr) {
		if (nFileContentBufferSize < nFileSize)
			throw ELibMCEnvInterfaceException(LIBMCENV_ERROR_BUFFERTOOSMALL);

        if (nFileSize > 0)
		    pStream->readBuffer (pFileContentBuffer, nFileSize, true);
	}
}


std::string CWorkingFile::CalculateSHA2()
{
    return AMCCommon::CUtils::calculateSHA256FromFile(m_sAbsolutePath);
}


IWorkingFileProcess* CWorkingFile::ExecuteFile()
{
	throw ELibMCEnvInterfaceException(LIBMCENV_ERROR_NOTIMPLEMENTED);
}


bool CWorkingFile::IsManaged()
{
    return m_pWorkingFileMonitor->fileIsMonitored (m_sFileName);
}

void CWorkingFile::MakeManaged()
{
    m_pWorkingFileMonitor->addNewMonitoredFile(m_sFileName);
}

bool CWorkingFile::FileExists()
{
    return AMCCommon::CUtils::fileOrPathExistsOnDisk(m_sAbsolutePath);
}

bool CWorkingFile::DeleteFromDisk()
{
    if (AMCCommon::CUtils::fileOrPathExistsOnDisk(m_sAbsolutePath)) {
        AMCCommon::CUtils::deleteFileFromDisk(m_sAbsolutePath, false);

        return !AMCCommon::CUtils::fileOrPathExistsOnDisk(m_sAbsolutePath);
    }

    return true;
}

