//-----------------------------------------------------------------------------
//           Name: im_message.h
//      Developer: Wolfire Games LLC
//    Description: Internal helper class for communication in adhoc GUIs as 
//                 part of the UI tools  
//        License: Read below
//-----------------------------------------------------------------------------
//
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
#pragma once

#include <GUI/IMUI/im_support.h>

class IMElement; // Forward declaration

/*******************************************************************************************/
/**
 * @brief A container for messages and its parameters
 *
 */
struct IMMessage
{
	
    std::string name;
	std::vector<std::string> stringParams;
	std::vector<int> intParams;
	std::vector<float> floatParams;
    
    int refCount; // for AS reference counting

	/*******************************************************************************************/
    /**
     * @brief  Constructor
     * 
     */
    IMMessage(std::string const& _name = "INVALID" ) :
        refCount(1)
    {
        IMrefCountTracker.addRefCountObject("Message");
        
		name = _name;
	}

	/*******************************************************************************************/
    /**
     * @brief  Copy constructor
     * 
     */
    IMMessage( IMMessage const& other )  :
        refCount(1)
    {
        IMrefCountTracker.addRefCountObject("Message");
        
        name = other.name;
        stringParams = other.stringParams;
        intParams = other.intParams;
        floatParams = other.floatParams;
    }
    
	// A few shortcuts for quickly filling in messages
	//  given that the majority of messages will have 0 or 1 parameters
    IMMessage(std::string const& _name, int param )  :
        refCount(1)
    {
        IMrefCountTracker.addRefCountObject("Message");
        
		name = _name;
		intParams.push_back( param );
	}

    IMMessage(std::string const& _name, float param )  :
        refCount(1)
    {
        IMrefCountTracker.addRefCountObject("Message");
        
		name = _name;
		floatParams.push_back( param );
	}

    IMMessage(std::string const& _name, std::string const& param )  :
        refCount(1)
    {
        IMrefCountTracker.addRefCountObject("Message");
        
		name = _name;
		stringParams.push_back( param );
	}
    
    void AddRef()
    {
        // Increase the reference counter
        refCount++;
    }
    
    void Release()
    {
        // Decrease ref count and delete if it reaches 0
        if( --refCount == 0 ) {
            delete this;
        }
    }
    
    static IMMessage* ASFactory( std::string const& name  ) {
        return new IMMessage(name);
    }
    
    static IMMessage* ASFactory_int( std::string const& name, int param  ) {
        return new IMMessage(name, param);
    }
    
    static IMMessage* ASFactory_float( std::string const& name, float param  ) {
        return new IMMessage(name, param);
    }
    
    static IMMessage* ASFactory_string( std::string const& name, std::string const& param  ) {
        return new IMMessage(name, param);
    }
    
    int numInts() {
        return (int) intParams.size();
    }
    
    int numFloats() {
        return (int) floatParams.size();
    }
    
    int numStrings() {
        return (int) stringParams.size();
    }
    
    void addInt( int param ) {
        intParams.push_back( param );
    }
    
    void addFloat( float param ) {
        floatParams.push_back( param );
    }
    
    void addString( std::string const& param ) {
        stringParams.push_back( param );
    }
    
    int getInt( int index ) {
        if( index < (int)intParams.size() ) {
            return intParams[ index ];
        }
        else {
            return 0;
        }
    }
    
    float getFloat( int index ) {
        if( index < (int)floatParams.size() ) {
            return floatParams[ index ];
        }
        else {
            return 0.0;
        }
    }
    
    std::string getString( int index ) {
        if( index < (int)stringParams.size() ) {
            return stringParams[ index ];
        }
        else {
            return "";
        }
    }
    
    ~IMMessage() {
        IMrefCountTracker.removeRefCountObject("Message");
    }
};


