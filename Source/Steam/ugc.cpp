//-----------------------------------------------------------------------------
//           Name: ugc.cpp
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
#include "ugc.h"

#include <Internal/integer.h>
#include <Internal/modid.h>

#include <Memory/allocation.h>
#include <Steam/ugc_item.h>
#include <Logging/logdata.h>

#if ENABLE_STEAMWORKS
#include <steam_api.h>
#endif

#include <cmath>

#if ENABLE_STEAMWORKS

SteamworksUGC::SteamworksUGC() :
m_callRemoteStoragePublishedFileSubscribed(this, &SteamworksUGC::OnUGCRemoteStoragePublishedFileSubscribed),
m_callRemoteStoragePublishedFileUnsubscribed(this, &SteamworksUGC::OnUGCRemoteStoragePublishedFileUnsubscribed),
m_callRemoteStoragePublishedFileDeleted(this, &SteamworksUGC::OnUGCRemoteStoragePublishedFileDeleted),
update_item(0)
{
    uint32_t nr_items = SteamUGC()->GetNumSubscribedItems();

    PublishedFileId_t* published_file_ids = static_cast<PublishedFileId_t*>(alloc.stack.Alloc(nr_items*sizeof(PublishedFileId_t)));

    uint32_t ret_items = SteamUGC()->GetSubscribedItems( published_file_ids, nr_items );

    for( unsigned i = 0; i < ret_items; i++ ) {
        LoadModIntoGame(published_file_ids[i]);
    }

    m_callRemoteStoragePublishedFileSubscribed.Register(this, &SteamworksUGC::OnUGCRemoteStoragePublishedFileSubscribed);
    m_callRemoteStoragePublishedFileUnsubscribed.Register(this, &SteamworksUGC::OnUGCRemoteStoragePublishedFileUnsubscribed);
    m_callRemoteStoragePublishedFileDeleted.Register(this, &SteamworksUGC::OnUGCRemoteStoragePublishedFileDeleted);
    alloc.stack.Free(published_file_ids);

    QueryPersonalWorkshopItems();
    QueryFavoritesWorkshopItems();
}

void SteamworksUGC::Update() {
    if(!items.empty()) {
        for(int end = update_item + int(ceilf(items.size() / (float)MAX_UPDATE_FRAMES)); update_item < end; ++update_item) {
            items[update_item % items.size()]->Update();
        }

        update_item %= items.size();
    }
}

UGCID SteamworksUGC::TryUploadMod( ModID sid ) {
    std::vector<SteamworksUGCItem*>::iterator pre_item = GetItem( sid );
    if( pre_item == GetItemEnd() ) {
        SteamworksUGCItem *item = new SteamworksUGCItem(sid);
        items.push_back(item);
        return item->GetUGCID();
    } else {
        LOGE << "Already have a mod with this sid, tied to a steamworks mod" << std::endl;
        return UGCID();
    }
}

std::vector<SteamworksUGCItem*>::iterator SteamworksUGC::LoadModIntoGame( PublishedFileId_t steamworks_id ) {
    std::vector<SteamworksUGCItem*>::iterator retit = GetItem( steamworks_id );
    if( retit == GetItemEnd() ) {
        uint32_t is = SteamUGC()->GetItemState(steamworks_id);
        if( is & k_EItemStateLegacyItem ) {
            LOGE << "Mod is a legacy mod, missing support for such, won't load" << std::endl;
        } else {
            items.push_back(new SteamworksUGCItem(steamworks_id));
            retit = items.begin()+(items.size()-1);
        }
    }
    return retit;
}

std::vector<SteamworksUGCItem*>::iterator SteamworksUGC::GetItem( PublishedFileId_t steamworks_id ) {
    std::vector<SteamworksUGCItem*>::iterator it = items.begin(); 
    for( ; it != items.end(); it++ ) {
        if( (*it)->steamworks_id == steamworks_id ) {
            break;
        }
    }
    return it;
}

std::vector<SteamworksUGCItem*>::iterator SteamworksUGC::GetItem( ModID sid ) {
    std::vector<SteamworksUGCItem*>::iterator it = items.begin(); 
    for( ; it != items.end(); it++ ) {
        if( (*it)->sid == sid ) {
            break;
        }
    }
    return it;
}

std::vector<SteamworksUGCItem*>::iterator SteamworksUGC::GetItem( UGCID ugc_id ) {
    std::vector<SteamworksUGCItem*>::iterator it = items.begin(); 
    for( ; it != items.end(); it++ ) {
        if( (*it)->ugc_id == ugc_id) {
            break;
        }
    }
    return it;
}

std::vector<SteamworksUGCItem*>::iterator SteamworksUGC::GetItemEnd() {
    return items.end();
}

std::vector<SteamworksUGCItem*>::iterator SteamworksUGC::GetItemBegin() {
    return items.begin();
}

size_t SteamworksUGC::SubscribedNotInstalledCount() {
    size_t ret = 0;
    for( unsigned i = 0; i < items.size(); i++ ) {
        if( items[i]->IsSubscribed() && items[i]->IsInstalled() == false ) {
            ret++;
        }
    }
    return ret;
}

size_t SteamworksUGC::DownloadingCount() {
    size_t ret = 0;
    for( unsigned i = 0; i < items.size(); i++ ) {
        if( items[i]->IsDownloading() ) {
            ret++;
        }
    }
    return ret;
}

size_t SteamworksUGC::DownloadPendingCount() {
    size_t ret = 0;
    for( unsigned i = 0; i < items.size(); i++ ) {
        if( items[i]->IsDownloadPending() ) {
            ret++;
        }
    }
    return ret;
}

size_t SteamworksUGC::NeedsUpdateCount() {
    size_t ret = 0;
    for( unsigned i = 0; i < items.size(); i++ ) {
        if( items[i]->NeedsUpdate() ) {
            ret++;
        }
    }
    return ret;
}

float SteamworksUGC::TotalDownloadProgress() {
    float total = 0.0f;
    int count = 0;
    for( unsigned i = 0; i < items.size(); i++ ) {
        if( items[i]->NeedsUpdate() ) {
            total += items[i]->ItemDownloadProgress();
            count++;
        }
    }
    return total/(float)count;
}

bool SteamworksUGC::WaitingForResults() {
    for( unsigned i = 0; i < items.size() ; i++ ) {
        if( items[i]->WaitingForResults() )
            return true;
    }
    return false;
}

void SteamworksUGC::QueryPersonalWorkshopItems() {
    CSteamID userid  = SteamUser()->GetSteamID(); 

    UGCQueryHandle_t query_handle = SteamUGC()->CreateQueryUserUGCRequest( 
        userid.GetAccountID(), 
        k_EUserUGCList_Published,
        k_EUGCMatchingUGCType_All,
        k_EUserUGCListSortOrder_TitleAsc,
        OVERGROWTH_APP_ID,
        OVERGROWTH_APP_ID,
        1  
    );


    SteamAPICall_t call = SteamUGC()->SendQueryUGCRequest( query_handle );
    m_callSteamUGCQueryCompleted.Set(call, this, &SteamworksUGC::OnUGCSteamUGCQueryCompleted);
}

void SteamworksUGC::QueryFavoritesWorkshopItems() {
    CSteamID userid  = SteamUser()->GetSteamID(); 

    std::vector<SteamworksUGCItem*>::iterator itemit;

    for( itemit = GetItemBegin(); itemit != GetItemEnd(); itemit++ ) {
        (*itemit)->favorite_clean_for_query = true;
    }

    UGCQueryHandle_t query_handle = SteamUGC()->CreateQueryUserUGCRequest( 
        userid.GetAccountID(), 
        k_EUserUGCList_Favorited,
        k_EUGCMatchingUGCType_All,
        k_EUserUGCListSortOrder_TitleAsc,
        OVERGROWTH_APP_ID,
        OVERGROWTH_APP_ID,
        1  
    );

    SteamAPICall_t call = SteamUGC()->SendQueryUGCRequest( query_handle );
    m_callSteamUGCQueryFavoritesCompleted.Set(call, this, &SteamworksUGC::OnUGCSteamUGCQueryFavoritesCompleted);
}

SteamworksUGC::~SteamworksUGC() {

}

void SteamworksUGC::OnUGCRemoteStoragePublishedFileSubscribed( RemoteStoragePublishedFileSubscribed_t *pCallback ) {
    LOGI << "Got notified about user subscribing to: " << pCallback->m_nPublishedFileId << std::endl;
    std::vector<SteamworksUGCItem*>::iterator pre_item = GetItem( pCallback->m_nPublishedFileId );
    if( pre_item == GetItemEnd() ) {
        LoadModIntoGame(pCallback->m_nPublishedFileId);
    } else {
        (*pre_item)->Reload();
    }
}

void SteamworksUGC::OnUGCRemoteStoragePublishedFileUnsubscribed( RemoteStoragePublishedFileUnsubscribed_t *pCallback ) {
    LOGI << "Got notified about user unsubscribing to: " << pCallback->m_nPublishedFileId << std::endl;

    std::vector<SteamworksUGCItem*>::iterator pre_item = GetItem( pCallback->m_nPublishedFileId );
    if( pre_item != GetItemEnd() ){
        (*pre_item)->Reload();
    }
}

void SteamworksUGC::OnUGCRemoteStoragePublishedFileDeleted( RemoteStoragePublishedFileDeleted_t *pCallback ) {
    LOGI << "Got notified deleted mod: " << pCallback->m_nPublishedFileId << std::endl;

    std::vector<SteamworksUGCItem*>::iterator pre_item = GetItem( pCallback->m_nPublishedFileId );
    if( pre_item != GetItemEnd() ){
        (*pre_item)->Reload();
    }
}

void SteamworksUGC::OnUGCSteamUGCQueryFavoritesCompleted( SteamUGCQueryCompleted_t *pResult, bool failed ) {
    LOGI << "OnUGCSteamUGCQueryFavoriteCompleted() " << pResult->m_eResult << std::endl;
    if( failed == false ) {
        std::vector<SteamworksUGCItem*>::iterator itemit;

        for( itemit = GetItemBegin(); itemit != GetItemEnd(); itemit++ ) {
            if( (*itemit)->favorite_clean_for_query ) {
                (*itemit)->favorite = false;
            }
        }

        for( unsigned i = 0; i < pResult->m_unNumResultsReturned; i++ ) {
            SteamUGCDetails_t pDetails;
            SteamUGC()->GetQueryUGCResult(pResult->m_handle, i, &pDetails);
            std::vector<SteamworksUGCItem*>::iterator modit = LoadModIntoGame(pDetails.m_nPublishedFileId);

            if( modit != GetItemEnd() ) {
                if( (*modit)->favorite_clean_for_query ) {
                    (*modit)->favorite = true;
                }
            }
        }
    } else {
        LOGE << "OnUGCSteamUGCQueryFavoritesCompleted() error " << pResult->m_eResult << std::endl;
    }

    if( pResult ) { 
        SteamUGC()->ReleaseQueryUGCRequest(pResult->m_handle);
    }
}

void SteamworksUGC::OnUGCSteamUGCQueryCompleted( SteamUGCQueryCompleted_t *pResult, bool failed ) {
    LOGI << "OnUGCSteamUGCQueryCompleted() " << pResult->m_eResult << std::endl;

    if( failed == false ) {
        for( unsigned i = 0; i < pResult->m_unNumResultsReturned; i++ ) {
            SteamUGCDetails_t pDetails;
            SteamUGC()->GetQueryUGCResult(pResult->m_handle, i, &pDetails);
            LoadModIntoGame(pDetails.m_nPublishedFileId);
        }
    } else {
        LOGE << "OnUGCSteamUGCQueryCompleted() error " << pResult->m_eResult << std::endl;
    }

    if( pResult ) { 
        SteamUGC()->ReleaseQueryUGCRequest(pResult->m_handle);
    }
}
#endif
