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


Abstract: This is a stub class definition of CNLightDriverBoard

*/

#include "libmcdriver_raylase_nlightdriverimpl.hpp"
#include "libmcdriver_raylase_interfaceexception.hpp"

#include "libmcdriver_raylase_apifield.hpp"

using namespace LibMCDriver_Raylase::Impl;


CNLightDriverImpl::CNLightDriverImpl(PRaylaseSDK pSDK, LibMCEnv::PDriverEnvironment pDriverEnvironment)
    : m_pSDK (pSDK), m_pDriverEnvironment (pDriverEnvironment)
{
    if (pSDK.get() == nullptr)
        throw ELibMCDriver_RaylaseInterfaceException(LIBMCDRIVER_RAYLASE_ERROR_INVALIDPARAM);
    if (pDriverEnvironment.get() == nullptr)
        throw ELibMCDriver_RaylaseInterfaceException(LIBMCDRIVER_RAYLASE_ERROR_INVALIDPARAM);
}

CNLightDriverImpl::~CNLightDriverImpl()
{

}


void CNLightDriverImpl::initializeNLightLaser(rlHandle cardHandle)
{

    /*m_pDriverEnvironment->LogMessage("Initializate nLights SPI mode..");
    CRaylaseAPIField_rlSpiConfig_v3 spiConfig;
    m_pSDK->checkError(m_pSDK->rlSfioSpiInitConfig((rlScannerConfig*)spiConfig.getData()));
    m_pSDK->checkError(m_pSDK->rlSfioSpiGetConfig(m_Handle, (rlScannerConfig*)spiConfig.getData()));
    spiConfig.setBool("Module2.Enabled", true);
    spiConfig.setEnumRaw("Module2.BitOrder", 0 );//"MsbFirst" );
    spiConfig.setDouble("Module2.PreDelay", 0.5);
    spiConfig.setDouble("Module2.PostDelay", 0.5);
    spiConfig.setDouble("Module2.FrameDelay", 0.25);
    spiConfig.setEnumRaw("Module2.SpiSyncMode", 1); //"SyncPerFrame"
    spiConfig.setDouble("Module2.ClockPeriod", 0.125);
    spiConfig.setInteger("Module2.BitsPerWord", 32);

    m_pSDK->checkError(m_pSDK->rlScannerSetConfig(m_Handle, (rlScannerConfig*)spiConfig.getData())); */

    m_pDriverEnvironment->LogMessage("Enabling nLight 24V...");
    m_pSDK->checkError(m_pSDK->rlGpioWrite(cardHandle, eRLIOPort::ioPortD, eRLPinAction::paSet, (uint32_t)eNlightDriverBoardIOPins::ENABLE_24V));

    m_pDriverEnvironment->LogMessage("Clearing nLight laser flags...");
    uint32_t nClearFlags = (uint32_t)eNlightDriverBoardIOPins::ARM_LASER | (uint32_t)eNlightDriverBoardIOPins::ENABLE_AIMING_LASER | (uint32_t)eNlightDriverBoardIOPins::GATE_IN;
    m_pSDK->checkError(m_pSDK->rlGpioWrite(cardHandle, eRLIOPort::ioPortD, eRLPinAction::paClear, nClearFlags));
    m_pDriverEnvironment->Sleep(1000);

    m_pDriverEnvironment->LogMessage("Enabling nLight external control");
    m_pSDK->checkError(m_pSDK->rlGpioWrite(cardHandle, eRLIOPort::ioPortD, eRLPinAction::paSet, (uint32_t)eNlightDriverBoardIOPins::ENABLE_EXTERNAL_CONTROL));
    m_pDriverEnvironment->Sleep(3000);
    //m_pSDK->checkError(m_pSDK->rlGpioWrite(m_Handle, eRLIOPort::ioPortD, eRLPinAction::paClear, (uint32_t)eNlightDriverBoardIOPins::ENABLE_EXTERNAL_CONTROL));

    m_pDriverEnvironment->LogMessage("Setting nLight system on..");
    m_pSDK->checkError(m_pSDK->rlGpioWrite(cardHandle, eRLIOPort::ioPortD, eRLPinAction::paClear, (uint32_t)eNlightDriverBoardIOPins::SYSTEM_ON));
    m_pDriverEnvironment->Sleep(3000);
    m_pSDK->checkError(m_pSDK->rlGpioWrite(cardHandle, eRLIOPort::ioPortD, eRLPinAction::paSet, (uint32_t)eNlightDriverBoardIOPins::SYSTEM_ON));
    m_pDriverEnvironment->Sleep(2000);

    m_pDriverEnvironment->LogMessage("Resetting beam profile");
    setNLightLaserMode(cardHandle, 0);

    //m_pDriverEnvironment->LogMessage("Setting aiming laser");
    //m_pSDK->checkError(m_pSDK->rlGpioWrite(m_Handle, eRLIOPort::ioPortD, eRLPinAction::paSet, (uint32_t)eNlightDriverBoardIOPins::ENABLE_AIMING_LASER));
    //m_pDriverEnvironment->Sleep(1000);
    //m_pSDK->checkError(m_pSDK->rlGpioWrite(m_Handle, eRLIOPort::ioPortD, eRLPinAction::paClear, (uint32_t)eNlightDriverBoardIOPins::ENABLE_AIMING_LASER));

    m_pDriverEnvironment->LogMessage("nLight Initialization done..");
}

void CNLightDriverImpl::disableNLightLaser(rlHandle cardHandle)
{
    m_pDriverEnvironment->LogMessage("Resetting beam profile");
    setNLightLaserMode(cardHandle, 0);

    m_pDriverEnvironment->LogMessage("Clearing nLight laser flags...");
    uint32_t nClearFlags = (uint32_t)eNlightDriverBoardIOPins::ARM_LASER | (uint32_t)eNlightDriverBoardIOPins::ENABLE_AIMING_LASER | (uint32_t)eNlightDriverBoardIOPins::GATE_IN;
    m_pSDK->checkError(m_pSDK->rlGpioWrite(cardHandle, eRLIOPort::ioPortD, eRLPinAction::paClear, nClearFlags));
    m_pDriverEnvironment->Sleep(1000);

    m_pDriverEnvironment->LogMessage("Disabling nLight external control");
    m_pSDK->checkError(m_pSDK->rlGpioWrite(cardHandle, eRLIOPort::ioPortD, eRLPinAction::paSet, (uint32_t)eNlightDriverBoardIOPins::ENABLE_EXTERNAL_CONTROL));
    m_pDriverEnvironment->Sleep(1000);

    m_pDriverEnvironment->LogMessage("Disabling nLight 24V...");
    m_pSDK->checkError(m_pSDK->rlGpioWrite(cardHandle, eRLIOPort::ioPortD, eRLPinAction::paClear, (uint32_t)eNlightDriverBoardIOPins::ENABLE_24V));

    m_pDriverEnvironment->LogMessage("nLight Laser deinitialized..");

}


void CNLightDriverImpl::clearNLightError(rlHandle cardHandle)
{
    m_pDriverEnvironment->LogMessage("Clearing nLight Error");
    m_pSDK->checkError(m_pSDK->rlGpioWrite(cardHandle, eRLIOPort::ioPortD, eRLPinAction::paClear, (uint32_t)eNlightDriverBoardIOPins::CLEAR_ERROR));
    m_pDriverEnvironment->Sleep(10);
    m_pSDK->checkError(m_pSDK->rlGpioWrite(cardHandle, eRLIOPort::ioPortD, eRLPinAction::paSet, (uint32_t)eNlightDriverBoardIOPins::CLEAR_ERROR));
    m_pDriverEnvironment->Sleep(1000);

}

void CNLightDriverImpl::setNLightLaserMode(rlHandle cardHandle, uint32_t nLaserMode)
{
    uint32_t nWriteDelay = 10;

    if (nLaserMode > RAYLASE_NLIGHT_MAXLASERMODE)
        throw ELibMCDriver_RaylaseInterfaceException(LIBMCDRIVER_RAYLASE_ERROR_INVALIDNLIGHTLASERMODE, "Invalid nLight laser mode: " + std::to_string(nLaserMode));

    uint32_t nSetMask = (uint32_t)eNlightDriverBoardIOPins::PRO_B7;
    uint32_t nClearMask = (uint32_t)eNlightDriverBoardIOPins::ENABLE_PROFILE;

    if ((nLaserMode & 1) != 0)
        nSetMask |= (uint32_t)eNlightDriverBoardIOPins::PRO_B1;
    else
        nClearMask |= (uint32_t)eNlightDriverBoardIOPins::PRO_B1;

    if ((nLaserMode & 2) != 0)
        nSetMask |= (uint32_t)eNlightDriverBoardIOPins::PRO_B2;
    else
        nClearMask |= (uint32_t)eNlightDriverBoardIOPins::PRO_B2;

    if ((nLaserMode & 4) != 0)
        nSetMask |= (uint32_t)eNlightDriverBoardIOPins::PRO_B3;
    else
        nClearMask |= (uint32_t)eNlightDriverBoardIOPins::PRO_B3;

    if ((nLaserMode & 8) != 0)
        nSetMask |= (uint32_t)eNlightDriverBoardIOPins::PRO_B4;
    else
        nClearMask |= (uint32_t)eNlightDriverBoardIOPins::PRO_B4;

    m_pSDK->checkError(m_pSDK->rlGpioWrite(cardHandle, eRLIOPort::ioPortD, eRLPinAction::paSet, nSetMask));
    m_pDriverEnvironment->Sleep(nWriteDelay);
    m_pSDK->checkError(m_pSDK->rlGpioWrite(cardHandle, eRLIOPort::ioPortD, eRLPinAction::paClear, nClearMask));
    m_pDriverEnvironment->Sleep(nWriteDelay);
    m_pSDK->checkError(m_pSDK->rlGpioWrite(cardHandle, eRLIOPort::ioPortD, eRLPinAction::paSet, (uint32_t)eNlightDriverBoardIOPins::PRO_START));
    m_pDriverEnvironment->Sleep(nWriteDelay);
    m_pSDK->checkError(m_pSDK->rlGpioWrite(cardHandle, eRLIOPort::ioPortD, eRLPinAction::paClear, (uint32_t)eNlightDriverBoardIOPins::PRO_START));
    m_pDriverEnvironment->Sleep(nWriteDelay);

}


void CNLightDriverImpl::addNLightLaserModeToList(rlListHandle listHandle, uint32_t nLaserMode)
{
    double dWriteDelay = 0.001;

    if (nLaserMode > RAYLASE_NLIGHT_MAXLASERMODE)
        throw ELibMCDriver_RaylaseInterfaceException(LIBMCDRIVER_RAYLASE_ERROR_INVALIDNLIGHTLASERMODE, "Invalid nLight laser mode: " + std::to_string(nLaserMode));

    uint32_t nSetMask = (uint32_t)eNlightDriverBoardIOPins::PRO_B7;
    uint32_t nClearMask = (uint32_t)eNlightDriverBoardIOPins::ENABLE_PROFILE;

    if ((nLaserMode & 1) != 0)
        nSetMask |= (uint32_t)eNlightDriverBoardIOPins::PRO_B1;
    else
        nClearMask |= (uint32_t)eNlightDriverBoardIOPins::PRO_B1;

    if ((nLaserMode & 2) != 0)
        nSetMask |= (uint32_t)eNlightDriverBoardIOPins::PRO_B2;
    else
        nClearMask |= (uint32_t)eNlightDriverBoardIOPins::PRO_B2;

    if ((nLaserMode & 4) != 0)
        nSetMask |= (uint32_t)eNlightDriverBoardIOPins::PRO_B3;
    else
        nClearMask |= (uint32_t)eNlightDriverBoardIOPins::PRO_B3;

    if ((nLaserMode & 8) != 0)
        nSetMask |= (uint32_t)eNlightDriverBoardIOPins::PRO_B4;
    else
        nClearMask |= (uint32_t)eNlightDriverBoardIOPins::PRO_B4;

    m_pSDK->checkError(m_pSDK->rlListAppendGpioValue(listHandle, eRLIOPort::ioPortD, eRLPinAction::paSet, nSetMask));
    m_pSDK->checkError(m_pSDK->rlListAppendSleep(listHandle, dWriteDelay));
    m_pSDK->checkError(m_pSDK->rlListAppendGpioValue(listHandle, eRLIOPort::ioPortD, eRLPinAction::paClear, nClearMask));
    m_pSDK->checkError(m_pSDK->rlListAppendSleep(listHandle, dWriteDelay));
    m_pSDK->checkError(m_pSDK->rlListAppendGpioValue(listHandle, eRLIOPort::ioPortD, eRLPinAction::paSet, (uint32_t)eNlightDriverBoardIOPins::PRO_START));
    m_pSDK->checkError(m_pSDK->rlListAppendSleep(listHandle, dWriteDelay));
    m_pSDK->checkError(m_pSDK->rlListAppendGpioValue(listHandle, eRLIOPort::ioPortD, eRLPinAction::paClear, (uint32_t)eNlightDriverBoardIOPins::PRO_START));
    m_pSDK->checkError(m_pSDK->rlListAppendSleep(listHandle, dWriteDelay));
}
