//-----------------------------------------------------------------------------
//           Name: versionxmlcreator.cpp
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
#include "versionxmlcreator.h"

#include <cstring>

#include <Ogda/jobhandler.h>
#include <tinyxml.h>
#include <XML/xml_helper.h>
#include <Version/version.h>
#include <Logging/logdata.h>
#include <Internal/filesystem.h>

ManifestResult VersionXMLCreator::Run(const JobHandler& jh, const Manifest& manifest) {
    std::string destination("version.xml");

    TiXmlDocument doc;
    // TiXmlDeclaration * decl = new TiXmlDeclaration( "2.0", "", "" );
    TiXmlElement* root = new TiXmlElement("release");

    {
        std::stringstream ss;

        ss << 100000 + GetBuildID();
        TiXmlElement* eVersion = new TiXmlElement("version");
        eVersion->LinkEndChild(new TiXmlText(ss.str().c_str()));
        root->LinkEndChild(eVersion);
    }

    {
        TiXmlElement* eName = new TiXmlElement("name");
        eName->LinkEndChild(new TiXmlText(GetBuildVersion()));
        root->LinkEndChild(eName);
    }

    {
        TiXmlElement* eShortname = new TiXmlElement("shortname");
        eShortname->LinkEndChild(new TiXmlText(GetBuildVersion()));
        root->LinkEndChild(eShortname);
    }

    {
        TiXmlElement* eDate = new TiXmlElement("date");
        eDate->LinkEndChild(new TiXmlText(GetBuildTimestamp()));
        root->LinkEndChild(eDate);
    }

    {
        TiXmlElement* eRev = new TiXmlElement("rev");
        eRev->LinkEndChild(new TiXmlText("9999"));
        root->LinkEndChild(eRev);
    }

    {
        TiXmlElement* eSvnrev = new TiXmlElement("svnrev");
        eSvnrev->LinkEndChild(new TiXmlText("9999"));
        root->LinkEndChild(eSvnrev);
    }

    {
        std::stringstream ss;
        ss << GetBuildID();
        std::string sss = ss.str();
        TiXmlElement* e = new TiXmlElement("build");
        e->LinkEndChild(new TiXmlText(sss.c_str()));
        root->LinkEndChild(e);
    }

    // doc.LinkEndChild( decl );
    doc.LinkEndChild(root);
    std::string full_name = AssemblePath(jh.output_folder, destination);
    doc.SaveFile(full_name.c_str());

    return ManifestResult(jh, destination, !doc.Error(), *this, "version");
}
