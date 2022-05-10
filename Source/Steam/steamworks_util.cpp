//-----------------------------------------------------------------------------
//           Name: steamworks_util.cpp
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
//   Copyright 2022 Wolfire Games LLC
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//-----------------------------------------------------------------------------
#include "steamworks_util.h"

#if ENABLE_STEAMWORKS
const char* GetEResultString(const EResult& error) {
    switch (error) {
        case k_EResultOK:
            return "success";
        case k_EResultFail:
            return "generic failure ";
        case k_EResultNoConnection:
            return "no/failed network connection";
        case k_EResultInvalidPassword:
            return "password/ticket is invalid";
        case k_EResultLoggedInElsewhere:
            return "same user logged in elsewhere";
        case k_EResultInvalidProtocolVer:
            return "protocol version is incorrect";
        case k_EResultInvalidParam:
            return "a parameter is incorrect";
        case k_EResultFileNotFound:
            return "file was not found";
        case k_EResultBusy:
            return "called method busy - action not taken";
        case k_EResultInvalidState:
            return "called object was in an invalid state";
        case k_EResultInvalidName:
            return "name is invalid";
        case k_EResultInvalidEmail:
            return "email is invalid";
        case k_EResultDuplicateName:
            return "name is not unique";
        case k_EResultAccessDenied:
            return "access is denied";
        case k_EResultTimeout:
            return "operation timed out";
        case k_EResultBanned:
            return "VAC2 banned";
        case k_EResultAccountNotFound:
            return "account not found";
        case k_EResultInvalidSteamID:
            return "steamID is invalid";
        case k_EResultServiceUnavailable:
            return "The requested service is currently unavailable";
        case k_EResultNotLoggedOn:
            return "The user is not logged on";
        case k_EResultPending:
            return "Request is pending (may be in process, or waiting on third party)";
        case k_EResultEncryptionFailure:
            return "Encryption or Decryption failed";
        case k_EResultInsufficientPrivilege:
            return "Insufficient privilege";
        case k_EResultLimitExceeded:
            return "Too much of a good thing";
        case k_EResultRevoked:
            return "Access has been revoked (used for revoked guest passes)";
        case k_EResultExpired:
            return "License/Guest pass the user is trying to access is expired";
        case k_EResultAlreadyRedeemed:
            return "Guest pass has already been redeemed by account, cannot be acked again";
        case k_EResultDuplicateRequest:
            return "The request is a duplicate and the action has already occurred in the past, ignored this time";
        case k_EResultAlreadyOwned:
            return "All the games in this guest pass redemption request are already owned by the user";
        case k_EResultIPNotFound:
            return "IP address not found";
        case k_EResultPersistFailed:
            return "failed to write change to the data store";
        case k_EResultLockingFailed:
            return "failed to acquire access lock for this operation";
        case k_EResultLogonSessionReplaced:
            return "Logon Session Replaced";
        case k_EResultConnectFailed:
            return "Connect Failed";
        case k_EResultHandshakeFailed:
            return "Handshake Failed";
        case k_EResultIOFailure:
            return "IO Failure";
        case k_EResultRemoteDisconnect:
            return "Remote Disconnect";
        case k_EResultShoppingCartNotFound:
            return "failed to find the shopping cart requested";
        case k_EResultBlocked:
            return "a user didn't allow it";
        case k_EResultIgnored:
            return "target is ignoring sender";
        case k_EResultNoMatch:
            return "nothing matching the request found";
        case k_EResultAccountDisabled:
            return "Account Disabled";
        case k_EResultServiceReadOnly:
            return "this service is not accepting content changes right now";
        case k_EResultAccountNotFeatured:
            return "account doesn't have value, so this feature isn't available";
        case k_EResultAdministratorOK:
            return "allowed to take this action, but only because requester is admin";
        case k_EResultContentVersion:
            return "A Version mismatch in content transmitted within the Steam protocol.";
        case k_EResultTryAnotherCM:
            return "The current CM can't service the user making a request, user should try another.";
        case k_EResultPasswordRequiredToKickSession:
            return "You are already logged in elsewhere, this cached credential login has failed.";
        case k_EResultAlreadyLoggedInElsewhere:
            return "You are already logged in elsewhere, you must wait";
        case k_EResultSuspended:
            return "Long running operation (content download) suspended/paused";
        case k_EResultCancelled:
            return "Operation canceled (typically by user: content download)";
        case k_EResultDataCorruption:
            return "Operation canceled because data is ill formed or unrecoverable";
        case k_EResultDiskFull:
            return "Operation canceled - not enough disk space.";
        case k_EResultRemoteCallFailed:
            return "an remote call or IPC call failed";
        case k_EResultPasswordUnset:
            return "Password could not be verified as it's unset server side";
        case k_EResultExternalAccountUnlinked:
            return "External account (PSN, Facebook...) is not linked to a Steam account";
        case k_EResultPSNTicketInvalid:
            return "PSN ticket was invalid";
        case k_EResultExternalAccountAlreadyLinked:
            return "External account (PSN, Facebook...) is already linked to some other account, must explicitly request to replace/delete the link first";
        case k_EResultRemoteFileConflict:
            return "The sync cannot resume due to a conflict between the local and remote files";
        case k_EResultIllegalPassword:
            return "The requested new password is not legal";
        case k_EResultSameAsPreviousValue:
            return "new value is the same as the old one ( secret question and answer )";
        case k_EResultAccountLogonDenied:
            return "account login denied due to 2nd factor authentication failure";
        case k_EResultCannotUseOldPassword:
            return "The requested new password is not legal";
        case k_EResultInvalidLoginAuthCode:
            return "account login denied due to auth code invalid";
        case k_EResultAccountLogonDeniedNoMail:
            return "account login denied due to 2nd factor auth failure - and no mail has been sent";
        case k_EResultHardwareNotCapableOfIPT:
            return "Hardware Not Capable Of IPT";
        case k_EResultIPTInitError:
            return "IPT Init Error";
        case k_EResultParentalControlRestricted:
            return "operation failed due to parental control restrictions for current user";
        case k_EResultFacebookQueryError:
            return "Facebook query returned an error";
        case k_EResultExpiredLoginAuthCode:
            return "account login denied due to auth code expired";
        case k_EResultIPLoginRestrictionFailed:
            return "IP Login Restriction Failed";
        case k_EResultAccountLockedDown:
            return "Account Locked Down";
        case k_EResultAccountLogonDeniedVerifiedEmailRequired:
            return "Account Logon Denied Verified Email Required";
        case k_EResultNoMatchingURL:
            return "No Matching URL";
        case k_EResultBadResponse:
            return "parse failure, missing field, etc.";
        case k_EResultRequirePasswordReEntry:
            return "The user cannot complete the action until they re-enter their password";
        case k_EResultValueOutOfRange:
            return "the value entered is outside the acceptable range";
        case k_EResultUnexpectedError:
            return "something happened that we didn't expect to ever happen";
        case k_EResultDisabled:
            return "The requested service has been configured to be unavailable";
        case k_EResultInvalidCEGSubmission:
            return "The set of files submitted to the CEG server are not valid !";
        case k_EResultRestrictedDevice:
            return "The device being used is not allowed to perform this action";
        case k_EResultRegionLocked:
            return "The action could not be complete because it is region restricted";
        case k_EResultRateLimitExceeded:
            return "Temporary rate limit exceeded, try again later, different from k_EResultLimitExceeded which may be permanent";
        case k_EResultAccountLoginDeniedNeedTwoFactor:
            return "Need two-factor code to login";
        case k_EResultItemDeleted:
            return "The thing we're trying to access has been deleted";
        case k_EResultAccountLoginDeniedThrottle:
            return "login attempt failed, try to throttle response to possible attacker";
        case k_EResultTwoFactorCodeMismatch:
            return "two factor code mismatch";
        case k_EResultTwoFactorActivationCodeMismatch:
            return "activation code for two-factor didn't match";
        case k_EResultAccountAssociatedToMultiplePartners:
            return "account has been associated with multiple partners";
        case k_EResultNotModified:
            return "data not modified";
        case k_EResultNoMobileDevice:
            return "the account does not have a mobile device associated with it";
        case k_EResultTimeNotSynced:
            return "the time presented is out of range or tolerance";
        case k_EResultSmsCodeFailed:
            return "SMS code failure (no match, none pending, etc.)";
        case k_EResultAccountLimitExceeded:
            return "Too many accounts access this resource";
        case k_EResultAccountActivityLimitExceeded:
            return "Too many changes to this account";
        case k_EResultPhoneActivityLimitExceeded:
            return "Too many changes to this phone";
        case k_EResultRefundToWallet:
            return "Cannot refund to payment method, must use wallet";
        case k_EResultEmailSendFailure:
            return "Cannot send an email";
        case k_EResultNotSettled:
            return "Can't perform operation till payment has settled";
        case k_EResultNeedCaptcha:
            return "Needs to provide a valid captcha";
        case k_EResultGSLTDenied:
            return "a game server login token owned by this token's owner has been banned";
        case k_EResultGSOwnerDenied:
            return "game server owner is denied for other reason (account lock, community ban, vac ban, missing phone)";
        case k_EResultInvalidItemType:
            return "the type of thing we were requested to act on is invalid";
        case k_EResultIPBanned:
            return "the ip address has been banned from taking this action";
        case k_EResultGSLTExpired:
            return "this token has expired from disuse; can be reset for use";
        default:
            return "";
    }
}

std::ostream& operator<<(std::ostream& data, const EResult& obj) {
    data << "\"Steamworks Error: " << GetEResultString(obj) << "\"";
    return data;
}
#endif
