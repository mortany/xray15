//**************************************************************************/
// Copyright (c) 2013 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/
// DESCRIPTION: Implementation
// AUTHOR: David Lanier
//***************************************************************************/

#include "NotificationAPIUtils.h"

// src/include
// Max SDK
#include <inode.h>
#include <imtl.h>
#include <render.h>
#include <iparamb.h>
#include <iparamb2.h>
#include <icustattribcontainer.h>
#include <custattrib.h>

using namespace Max::NotificationAPI;
using namespace MaxSDK::NotificationAPI;

void Utils::DebugPrintToFileEventType(FILE* file, NotifierType notifierType, size_t updateType, size_t indent)
{
    if (! file){
        DbgAssert(0 && _T("file is NULL"));
        return;
    }

    TSTR indentString = _T("");
    for (size_t i=0;i<indent;++i){
        indentString += TSTR(_T("\t"));
    }
    _ftprintf(file, indentString);
    _ftprintf(file, _T("** Supported Update Type : **\n"));

    ++indent;
    indentString += TSTR(_T("\t"));

    switch(notifierType){
        case NotifierType_Node_Camera:
        case NotifierType_Node_Light:
        case NotifierType_Node_Helper:
        case NotifierType_Node_Geom:
        {
            //Is a node
            NodeEventType nodeEventType = static_cast<NodeEventType>(updateType);
            switch(nodeEventType){
                case EventType_Node_ParamBlock:{
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  Node ParamBlock **\n"));
                }
                break;
                case EventType_Node_Uncategorized:{
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  Node Uncategorized **\n"));
                }
                break;
                case EventType_Node_WireColor:{
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  Node WireColor **\n"));
                }
                break;
                case EventType_Node_Selection:
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  Node Selection **\n"));
                    break;
		        case EventType_Node_Transform:{
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  Node Transform **\n"));
                }
                break;
		        case EventType_Node_Deleted:{
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  Node Deleted **\n"));
                }
                break;
		        case EventType_Node_Hide:{
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  Node Hide **\n"));
                }
                break;
                case EventType_Node_Material_Replaced:{
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  Node Material Replaced **\n"));
                }
                break;
		        case EventType_Node_Material_Updated:{
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  Node Material parameter changed **\n"));
                }
                break;
                case EventType_Node_Reference:{
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  Node reference changed **\n"));
                }
                break;
		        case EventType_Mesh_Vertices:{
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  Mesh Vertices **\n"));
                }
                break;
		        case EventType_Mesh_Faces:{
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  Mesh Faces **\n"));
                }
                break;
		        case EventType_Mesh_UVs:{
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  Mesh UVs **\n"));
                }
                break;
		        default:{
                    DbgAssert(0 && _T("Unknown node update "));
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  Unknown node update ! **\n"));
                }
                break;
            }
        }
        break;
		
		case NotifierType_Material:{
             MaterialEventType matEventType = static_cast<MaterialEventType>(updateType);
            switch(matEventType){
                case EventType_Material_ParamBlock:{
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  Material ParamBlock **\n"));
                };
                break;
                case EventType_Material_Deleted:{
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  Material Deleted **\n"));
                };
                break;
                case EventType_Material_Reference:{
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  Material Reference **\n"));
                };
                break;
                case EventType_Material_Uncategorized:{
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  Material Uncategorized **\n"));
                };
                break;
                default:{
                    DbgAssert(0 && _T("Unknown mtl update "));
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  Unknown material update ! **\n"));
               }
               break;
            }
        }
        break;

        case NotifierType_Texmap:
            {
                TexmapEventType texmapEventType = static_cast<TexmapEventType>(updateType);
                _ftprintf(file, indentString);
                switch(texmapEventType)
                {
                case EventType_Texmap_ParamBlock:
                    _ftprintf(file, _T("**  Texmap ParamBlock **\n"));
                    break;
                case EventType_Texmap_Deleted:
                    _ftprintf(file, _T("**  Texmap Deleted **\n"));
                    break;
                case EventType_Texmap_Uncategorized:
                    _ftprintf(file, _T("**  Texmap Uncategorized **\n"));
                    break;
                default:
                    DbgAssert(0 && _T("Unknown texmap update "));
                    _ftprintf(file, _T("**  Unknown texmap update ! **\n"));
                    break;
                }
            }
        break;

        case NotifierType_View:{
            const ViewEventType vportEventType = static_cast<ViewEventType>(updateType);
            switch(vportEventType){
                case EventType_View_Properties:
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  viewport properties **\n"));
                break;
                case EventType_View_Active:
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  viewport active **\n"));
                    break;
                case EventType_View_Deleted:
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  viewport deleted **\n"));
                    break;
                default:{
                    DbgAssert(0 && _T("Unknown viewport update "));
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  Unknown viewport update ! **\n"));
                }
                break;
            }
        }
        break;

        case NotifierType_RenderSettings:{
            RenderSettingsEventType rsEventType = static_cast<RenderSettingsEventType>(updateType);
            switch(rsEventType){
                case EventType_RenderSettings_LockView:{
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  Render settings LockView **\n"));
                };
                break;
                default:{
                    DbgAssert(0 && _T("Unknown render settings update "));
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  Unknown render settings event ! **\n"));
                }
                break;
            }
        }
        break;

        case NotifierType_SceneNode:{
            SceneEventType sceneEventType = static_cast<SceneEventType>(updateType);
            switch(sceneEventType){
                case EventType_Scene_Node_Added:{
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  Scene Node Added **\n"));
                }
                break;
                default:{
                    DbgAssert(0 && _T("Unknown scene update "));
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("**  Unknown scene update ! **\n"));
                }
                break;
            }
        }
        break;
	
        default:{
            DbgAssert(0 && _T("Unknown notifier type"));
            _ftprintf(file, indentString);
            _ftprintf(file, _T("**  Unknown notifier type ! **\n"));
        }
        break;	
    }
}

void Max::NotificationAPI::Utils::DebugPrintToFileNotifierType(FILE* file, NotifierType type, RefTargetHandle refTargetHandle, size_t indent)
{
    static const TCHAR* s_InvalidPointer = _T("invalid pointer");

    if (! file){
        DbgAssert(0 && _T("file is NULL"));
        return;
    }

    TSTR indentString = _T("");
    for (size_t i=0;i<indent;++i){
        indentString += TSTR(_T("\t"));
    }

    //Some refTargetHandle maybe be NULL

    switch(type){
		case NotifierType_Node_Camera:{
            _ftprintf(file, indentString);
			_ftprintf(file, _T("** Type : Camera, name : \"%s\" **\n"),(NULL != refTargetHandle)?(static_cast<INode*>(refTargetHandle)->GetName()):s_InvalidPointer);
		}
		break;
		case NotifierType_Node_Light:{
            _ftprintf(file, indentString);
			_ftprintf(file, _T("** Type : Light, name : \"%s\" **\n"),(NULL != refTargetHandle)?(static_cast<INode*>(refTargetHandle)->GetName()):s_InvalidPointer);
		}
		break;
		case NotifierType_Node_Helper:{
            _ftprintf(file, indentString);
			_ftprintf(file, _T("** Type : Helper, name : \"%s\" **\n"),(NULL != refTargetHandle)?(static_cast<INode*>(refTargetHandle)->GetName()):s_InvalidPointer);
		}
		break;
		case NotifierType_Node_Geom:{
            _ftprintf(file, indentString);
			_ftprintf(file, _T("** Type : Geom node, name : \"%s\" **\n"),(NULL != refTargetHandle)?(static_cast<INode*>(refTargetHandle)->GetName()):s_InvalidPointer);
		}
		break;
		case NotifierType_Material:{
            _ftprintf(file, indentString);
			_ftprintf(file, _T("** Type : Material , name : \"%s\" **\n"),(NULL != refTargetHandle)?(static_cast<MtlBase*>(refTargetHandle)->GetName().data()):s_InvalidPointer);
		}
		break;
		case NotifierType_Texmap:{
            _ftprintf(file, indentString);
			_ftprintf(file, _T("** Type : Texmap, name : \"%s\" **\n"),(NULL != refTargetHandle)?(static_cast<MtlBase*>(refTargetHandle)->GetName().data()):s_InvalidPointer);
		}
		break;
		case NotifierType_View:{
            _ftprintf(file, indentString);
			_ftprintf(file, _T("** Type : View **\n"));
		}
		break;
        case NotifierType_RenderSettings:{
            _ftprintf(file, indentString);
			_ftprintf(file, _T("** Type : Render settings **\n"));
		}
		break;
        case NotifierType_SceneNode:{
            _ftprintf(file, indentString);
			_ftprintf(file, _T("** Type : Scene **\n"));
		}
		break;
		default:{
            _ftprintf(file, indentString);
			_ftprintf(file, _T("** Type : Unknown !! **\n"));
		}
		break;
	}
	
}

void Max::NotificationAPI::Utils::DebugPrintToFileMonitoredEvents (FILE* file,    NotifierType notifierType, size_t monitoredEvents, size_t indent)
{
    if (! file){
        DbgAssert(0 && _T("file is NULL"));
        return;
    }

    TSTR indentString = _T("");
    for (size_t i=0;i<indent;++i){
        indentString += TSTR(_T("\t"));
    }

    _ftprintf(file, indentString);
    _ftprintf(file, _T("** Supported Update Type : **\n"));

    ++indent;
    indentString += TSTR(_T("\t"));

    switch(notifierType){
        case NotifierType_Node_Camera:
        case NotifierType_Node_Light:
        case NotifierType_Node_Helper:
        case NotifierType_Node_Geom:
        {
            //Is a node
            if (monitoredEvents & EventType_Node_ParamBlock){
                _ftprintf(file, indentString);
                _ftprintf(file, _T("** Node ParamBlock **\n"));
            }

            if (monitoredEvents & EventType_Node_WireColor){
                _ftprintf(file, indentString);
                _ftprintf(file, _T("** Node WireColor **\n"));
            }

            if (monitoredEvents & EventType_Node_Selection){
                _ftprintf(file, indentString);
                _ftprintf(file, _T("** Node Selection **\n"));
            }

            if (monitoredEvents & EventType_Node_Transform){
                _ftprintf(file, indentString);
                _ftprintf(file, _T("** Node Transform **\n"));
            }

            if (monitoredEvents & EventType_Node_Deleted){
                _ftprintf(file, indentString);
                _ftprintf(file, _T("** Node Deleted **\n"));
            }
            
            if (monitoredEvents & EventType_Node_Hide){
                _ftprintf(file, indentString);
                _ftprintf(file, _T("** Node Hide **\n"));
            }

            if (monitoredEvents & EventType_Node_Material_Replaced){
                _ftprintf(file, indentString);
                _ftprintf(file, _T("** Node Material Replaced **\n"));
            }   

            if (monitoredEvents & EventType_Node_Material_Updated){
                _ftprintf(file, indentString);
                _ftprintf(file, _T("** Node Material Parameter Changed **\n"));
            }

            if (monitoredEvents & EventType_Mesh_Vertices){
                _ftprintf(file, indentString);
                _ftprintf(file, _T("** Mesh Vertices **\n"));
            }
                
		    if (monitoredEvents & EventType_Mesh_Faces){
                _ftprintf(file, indentString);
                _ftprintf(file, _T("** Mesh Faces **\n"));
            }
                
		    if (monitoredEvents & EventType_Mesh_UVs){
                _ftprintf(file, indentString);
                _ftprintf(file, _T("** Mesh UVs **\n"));
            }
        }
        break;
		
		case NotifierType_Material:{

            if (monitoredEvents & EventType_Material_ParamBlock){
                _ftprintf(file, indentString);
                _ftprintf(file, _T("** Material ParamBlock **\n"));
            }
        }
        break;

        case NotifierType_Texmap:{

            if (monitoredEvents & EventType_Texmap_ParamBlock){
                _ftprintf(file, indentString);
                _ftprintf(file, _T("** Texmap ParamBlock **\n"));
            }
            if (monitoredEvents & EventType_Texmap_Deleted){
                _ftprintf(file, indentString);
                _ftprintf(file, _T("** Texmap Deleted **\n"));
            }
            if (monitoredEvents & EventType_Texmap_Uncategorized){
                _ftprintf(file, indentString);
                _ftprintf(file, _T("** Texmap Uncategorized **\n"));
            }
        }
        break;

        case NotifierType_View:
            if (monitoredEvents & EventType_View_Properties){
                _ftprintf(file, indentString);
                _ftprintf(file, _T("** view properties **\n"));
            }
            if (monitoredEvents & EventType_View_Active){
                _ftprintf(file, indentString);
                _ftprintf(file, _T("** view active **\n"));
            }
            if (monitoredEvents & EventType_View_Deleted){
                _ftprintf(file, indentString);
                _ftprintf(file, _T("** view deleted **\n"));
            }
        break;

        case NotifierType_RenderSettings:{
            if (monitoredEvents & EventType_RenderSettings_LockView){
                _ftprintf(file, indentString);
                _ftprintf(file, _T("** Render Settings Lock view **\n"));
            }
        }
        break;

        case NotifierType_SceneNode:{
            if (monitoredEvents & EventType_Scene_Node_Added){
                _ftprintf(file, indentString);
                _ftprintf(file, _T("** Scene Node Added **\n"));
            }
        }
        break;
	
        default:{
            _ftprintf(file, indentString);
            _ftprintf(file, _T("** Unknown notifier type ! **\n"));
        }
        break;	
    }

    
}

INode* Max::NotificationAPI::Utils::GetViewNode(ViewExp &viewportExp)
{
	INode* viewNode = NULL;

	if(viewportExp.GetViewCamera())
	{
		viewNode = viewportExp.GetViewCamera();
	}else if(viewportExp.GetViewSpot())
	{
		viewNode = viewportExp.GetViewSpot();
	}

	return viewNode;
}

INode* Max::NotificationAPI::Utils::GetViewNode(ViewExp *viewportExp)
{
    if(viewportExp != nullptr)
    {
        return GetViewNode(*viewportExp);
    }
    else
    {
        return nullptr;
    }
}

namespace Max{
namespace NotificationAPI{

static const MSTR s_EmptyString(_T(""));

static bool s_GetParamNameFromParamBlock1(MSTR& outParamName, RefTargetHandle , IParamBlock* pBlock1, int paramIndex)
{
    outParamName = s_EmptyString;
    if (paramIndex < 0 || NULL == pBlock1){
        DbgAssert(0 &&_T("index < 0 || NULL == pBlock1"));
        return false;
    }

    GetParamName gpn (TSTR(_T("")), paramIndex);
    pBlock1->NotifyDependents(FOREVER, (PartID)&gpn, REFMSG_GET_PARAM_NAME, NOTIFY_ALL, FALSE);

    outParamName = gpn.name;
    return true;   
}

static void s_GetLastUpdatedParamFromReference(bool& outResult, RefTargetHandle pRefTargetHdl, int refIndex, ParamBlockData& outParamblockData)
{
    if (true == outResult){
        return; //Break any recursion when success
    }

    if( NULL == pRefTargetHdl){
        DbgAssert(0 &&_T("pRefTargetHdl is NULL"));
        outResult = false;
        return;
    }
    //Take partA of ClassID and check if it's a paramblock and a pblock 1 or 2
    const ULONG ClassIDPartA = pRefTargetHdl->ClassID().PartA();
    switch( ClassIDPartA ){
        case PARAMETER_BLOCK2_CLASS_ID:{//ParamBlock 2
            IParamBlock2* pblock2       = static_cast<IParamBlock2*>(pRefTargetHdl);
            const int lastNotifyParamID = pblock2->LastNotifyParamID();//Is an ID (not an index)

            if (-1 != lastNotifyParamID){
                //Found
                outParamblockData.m_ParamBlockType  = ParamBlockData::PB_TWO;
                ParamBlockData::ContainerTypeAndIndex containerTypeandIndex;
                containerTypeandIndex.m_Index       = refIndex;
                containerTypeandIndex.m_Type        = ParamBlockData::REFERENCES; //This data comes from the references and the #refIndex ref.
                outParamblockData.m_ParamBlockIndexPath.append(containerTypeandIndex);
                outParamblockData.m_ParametersIDsUpdatedInThatParamBlock.append(lastNotifyParamID);
                const ParamDef& pdef                = pblock2->GetParamDef(lastNotifyParamID);
                outParamblockData.m_ParametersNames.append(MSTR(pdef.int_name));
                outResult = true;
                return;
            }
        }break;
        case PARAMETER_BLOCK_CLASS_ID:{//ParamBlock 1
            IParamBlock* pblock1        = static_cast<IParamBlock*>(pRefTargetHdl);
            const int lastNotifyIndex   = pblock1->LastNotifyParamNum(); //Is an index (not an ID)
            if (-1 != lastNotifyIndex){
                //Found
                outParamblockData.m_ParamBlockType  = ParamBlockData::PB_ONE;
                ParamBlockData::ContainerTypeAndIndex containerTypeandIndex;
                containerTypeandIndex.m_Index       = refIndex;
                containerTypeandIndex.m_Type        = ParamBlockData::REFERENCES; //This data comes from the references and the #refIndex ref.
                outParamblockData.m_ParamBlockIndexPath.append(containerTypeandIndex);
                outParamblockData.m_ParametersIDsUpdatedInThatParamBlock.append(lastNotifyIndex);
                MSTR paramBlockName(_T(""));
                const bool bFound = s_GetParamNameFromParamBlock1(paramBlockName, pRefTargetHdl, pblock1, lastNotifyIndex);
                if (bFound){
                    outParamblockData.m_ParametersNames.append(paramBlockName);
                }
                else{
                    outParamblockData.m_ParametersNames.append(MSTR(_T("PBLOCK1")));
                    DbgAssert(0 && _T("param name from ParamBlock 1 could not be retrieved !"));
                }
                outResult = true;
                return;
            }
        }
        break;
        default:{
            //Is that a ShadowType from a light ?
            const SClass_ID& sClassID = pRefTargetHdl->SuperClassID();
            switch (sClassID){
                case SHADOW_TYPE_CLASS_ID:{
                    //It's a Shadow generator plugin as a reference, so check if one of it's param block has been updated
               
                    ParamBlockData::ContainerTypeAndIndex containerTypeandIndex;
                    containerTypeandIndex.m_Index   = refIndex;
                    containerTypeandIndex.m_Type    = ParamBlockData::REFERENCES; //This data comes from the references and the #refIndex ref.

                    //We add the index from the references from this RefTargetHandle
                    outParamblockData.m_ParamBlockIndexPath.append(containerTypeandIndex);

                    //Recurse
                    Max::NotificationAPI::Utils::GetLastUpdatedParamFromObject(outResult, *pRefTargetHdl, outParamblockData);
                    if (true == outResult){
                        return;
                    }

                    //In case we failed finding the Last updated param, remove the index from path
                    outParamblockData.m_ParamBlockIndexPath.removeLast();
                }
                break;
                case SHADER_CLASS_ID:{ //Shader from Std Material containing some pblocks in its references
                    const int numRefsFromShader = pRefTargetHdl->NumRefs();
                    for(int iRef = 0; iRef < numRefsFromShader; ++iRef) {
		                RefTargetHandle pRefTarget = pRefTargetHdl->GetReference(iRef);
		                if(NULL != pRefTarget) {
                            //Enter the path from custom attributes and remove it if we fail finding the last notify parameter in its references
                            ParamBlockData::ContainerTypeAndIndex containerTypeandIndex;
                            containerTypeandIndex.m_Index   = refIndex;//Add the attribute index
                            containerTypeandIndex.m_Type    = ParamBlockData::SHADER; //This data comes from the references and the #refIndex ref.

                            //We add the index from the references from this RefTargetHandle for the path
                            outParamblockData.m_ParamBlockIndexPath.append(containerTypeandIndex);

                            s_GetLastUpdatedParamFromReference(outResult, pRefTarget, iRef, outParamblockData);
                            if (true == outResult){
                                return;
                            }

                            //In case we failed finding the Last updated param, remove the index from path
                            outParamblockData.m_ParamBlockIndexPath.removeLast();
		                }//end of if(NULL != pRefTarget) {
	                }//end of for loop
                };
                break;
                default:{
                    //Ignore
                }
                break;
            }
        }
        break;
    }//end of switch
}//end of function
}//end of namespace Max
}//end of namespace NotificationAPI

void Max::NotificationAPI::Utils::GetLastUpdatedParamFromObject(bool& outResult, ReferenceTarget& referenceTarget, ParamBlockData& outParamblockData)
{
    if(true == outResult){
        return; //Break recursion
    }

    outResult                           = false;
    outParamblockData.m_ParametersNames.removeAll();
    outParamblockData.m_ParamBlockType  = ParamBlockData::UNKNOWN_PB_TYPE;
    DbgAssert(outParamblockData.m_ParametersIDsUpdatedInThatParamBlock.length() <= 0);
    outParamblockData.m_ParametersIDsUpdatedInThatParamBlock.removeAll();
    //we don't empty outParamblockData.m_ParamBlockIndexPath as this function is sometimes recursive and may already contain some paths indices
    
    //We are going to iterate through the references and look for paramblocks as obj->GetParamBlock(int i) is not always implemented to make the Paramblocks public
    const size_t numReferences  = referenceTarget.NumRefs();
    for (int iRef=0;iRef<numReferences;++iRef){
        RefTargetHandle pSubRefTarg = referenceTarget.GetReference(iRef);
        if (NULL == pSubRefTarg){
            continue; //It happens to have NULL references
        }

        s_GetLastUpdatedParamFromReference(outResult, pSubRefTarg, iRef, outParamblockData);
        if (true == outResult){
            return;
        }
    }//end of for loop

    //Nothing found in references, so check the references from custom attributes as lights have some of them for mental ray and they contain paramblocks
    ICustAttribContainer* custAttribCont = referenceTarget.GetCustAttribContainer();
    if(NULL != custAttribCont) {
	    const int numCustAttr = custAttribCont->GetNumCustAttribs();
	    for(int iAttr = 0; iAttr < numCustAttr; ++iAttr) {
		    CustAttrib* custAttrib = custAttribCont->GetCustAttrib(iAttr);
		    if(NULL != custAttrib) {
                const int numRefs = custAttrib->NumRefs();
                for (int iRef=0;iRef<numRefs;++iRef){
                    RefTargetHandle pSubRefTarg = custAttrib->GetReference(iRef);
                    if (NULL == pSubRefTarg){
                        continue;
                    }

                    //Enter the path from custom attributes and remove it if we fail finding the last notify parameter in its references
                    ParamBlockData::ContainerTypeAndIndex containerTypeandIndex;
                    containerTypeandIndex.m_Index   = iAttr;//Add the attribute index
                    containerTypeandIndex.m_Type    = ParamBlockData::CUSTOM_ATTRIBUTES; //This data comes from the references and the #refIndex ref.

                    //We add the index from the references from this RefTargetHandle
                    outParamblockData.m_ParamBlockIndexPath.append(containerTypeandIndex);

                    s_GetLastUpdatedParamFromReference(outResult, pSubRefTarg, iRef, outParamblockData);
                    if (true == outResult){
                        return;
                    }

                    //In case we failed finding the Last updated param, remove the index from path
                    outParamblockData.m_ParamBlockIndexPath.removeLast();
                }//end of for loop
		    }//end of if(NULL != custAttrib) {
	    }//end of for loop
    }//end of if(NULL != custAttribCont)

    //Not found
}

//Merge content of otherParamBlockData into inoutParamBlockData
void Max::NotificationAPI::Utils::MergeParamBlockDatas(MaxSDK::Array<ParamBlockData>& inoutParamBlockDatas, const MaxSDK::Array<ParamBlockData>& otherParamBlockDatas)
{
    const size_t numOtherPblockData = otherParamBlockDatas.length();
    const size_t numPblockData      = inoutParamBlockDatas.length();

    //2 cases for each pblockdata :
    //1) the index from a pblockdata from otherParamBlockDatas is already inside a pblockdata from inoutParamBlockDatas
    //  In that case we are adding the indices from m_ParametersIDsUpdatedInThatParamBlock to the other pblockdata taking care of not duplicating the IDs (they must be unique)
    //2) the index is not inside the array of ParamIDs so we need to add it to the array of pblockdata

    //Iterate through the others pblock datas, we are going to try to merge them
    for(int iOtherData=0;iOtherData<numOtherPblockData;++iOtherData){
        const ParamBlockData& otherData                                                 = otherParamBlockDatas[iOtherData];
        const MaxSDK::Array<ParamBlockData::ContainerTypeAndIndex>& otherIndicesPath    = otherData.m_ParamBlockIndexPath;
        const ParamBlockData::ParameterBlockType otherType                              = otherData.m_ParamBlockType;
        const MaxSDK::Array<int>& otherParamIDs                                         = otherData.m_ParametersIDsUpdatedInThatParamBlock;
        const size_t numOthersParamIDs                                                  = otherParamIDs.length();
        const size_t numOtherParamNames                                                 = otherData.m_ParametersNames.length();
        if (numOthersParamIDs <= 0){
            DbgAssert(0 &&_T("numOthersParamIDs is negative or null"));
            continue;
        }

        bool bOtherDataIndexWasFoundInInOutParamBlockDatas = false;
        
        //Iterate through the inoutParamBlockDatas to see if the indices path of this ParamBlockata named otherData is already referenced
        for(int iData=0;iData<numPblockData;++iData){
            ParamBlockData& data                                                = inoutParamBlockDatas[iData];
            MaxSDK::Array<ParamBlockData::ContainerTypeAndIndex>& indicesPath   = data.m_ParamBlockIndexPath;
            const ParamBlockData::ParameterBlockType type                       = data.m_ParamBlockType;
            if (indicesPath == otherIndicesPath && type == otherType){//Array of ContainerTypeAndIndex comparison and PB type comparison
                //It's the same paramblock type, and indices paths are the same, so add these Ids if not already inside the array
                MaxSDK::Array<int>& paramIDs = data.m_ParametersIDsUpdatedInThatParamBlock;

                //Merge names if unique
                for (size_t iName=0;iName<numOtherParamNames;++iName){
                    const MSTR& otherParamName = otherData.m_ParametersNames[iName];
                    if (false == data.m_ParametersNames.contains(otherParamName)){
                        //Add it to the array
                        data.m_ParametersNames.append(otherParamName);
                    }
                }

                //Append all indices from otherParamIDs that are not in paramIDs
                for (int iOtherParam=0;iOtherParam<numOthersParamIDs;++iOtherParam){
                    const int currentOtherParamID = otherParamIDs[iOtherParam];
                    if (paramIDs.contains(currentOtherParamID)){
                        continue; //Ignore this index, it's already inside the other array
                    }

                    //The index currentOtherParamID is not inside paramIDs, so add it
                    paramIDs.append(currentOtherParamID);
                }
                bOtherDataIndexWasFoundInInOutParamBlockDatas = true;
                break;
            }
        }

        if (false == bOtherDataIndexWasFoundInInOutParamBlockDatas){
            //We have iterated through the inoutParamBlockDatas array but did not find a matching indices paths, so add this pblockdata to the inoutParamBlockDatas array
            inoutParamBlockDatas.append(otherData);
        }
    }
}

void Max::NotificationAPI::Utils::DebugPrintParamBlockData(FILE* file, const ParamBlockData& pblockData, size_t indent)
{
    if (! file){
        DbgAssert(0 && _T("file is NULL"));
        return;
    }
    
    TSTR indentString = _T("");
    for (size_t i=0;i<indent;++i){
        indentString += TSTR(_T("\t"));
    }

    _ftprintf(file, indentString);
    switch (pblockData.m_ParamBlockType){
        case ParamBlockData::PB_ONE:
            _ftprintf(file, _T("** Paramblock type : PB1 **\n") );
        break;
        case ParamBlockData::PB_TWO:
            _ftprintf(file, _T("** Paramblock type : PB2 **\n") );
        break;
        default:
            _ftprintf(file, _T("\n** ERROR : paramblock type : UNKNOWN !! **\n") );
        break;
    }
    
    const TSTR indentStringPlus     = indentString      + TSTR(_T("\t"));
    const TSTR indentStringPlusPlus = indentStringPlus  + TSTR(_T("\t"));

    _ftprintf(file, indentString);
    const size_t numIndicesPaths = pblockData.m_ParamBlockIndexPath.length();
    _ftprintf(file, _T("** Num indices paths : %llu **\n"), numIndicesPaths);
    
    for(size_t i=0;i<numIndicesPaths;++i){
        _ftprintf(file, indentStringPlus);
        const ParamBlockData::ContainerTypeAndIndex& containerTypeAndIndex = pblockData.m_ParamBlockIndexPath[i];
        _ftprintf(file, _T("** Index paths in references #%llu is : **\n"), i);
        _ftprintf(file, indentStringPlusPlus);
        switch(containerTypeAndIndex.m_Type){
            case ParamBlockData::CUSTOM_ATTRIBUTES:
                _ftprintf(file, _T("** Container type : Custom Attributes **\n"));
                _ftprintf(file, indentStringPlusPlus);
                _ftprintf(file, _T("** Container index : %d **\n"), containerTypeAndIndex.m_Index);
            break;
            case ParamBlockData::REFERENCES:
                _ftprintf(file, _T("** Container type : References **\n"));
                _ftprintf(file, indentStringPlusPlus);
                _ftprintf(file, _T("** Container index : %d **\n"), containerTypeAndIndex.m_Index);
            break;
            case ParamBlockData::SHADER:
                _ftprintf(file, _T("** Container type : Shader **\n"));
                _ftprintf(file, indentStringPlusPlus);
                _ftprintf(file, _T("** Container index : %d **\n"), containerTypeAndIndex.m_Index);
            break;
            default:
                _ftprintf(file, _T("** ERROR ParamBlockData container type : UNKNONWN **\n"));
                _ftprintf(file, indentStringPlusPlus);
                _ftprintf(file, _T("** Container index : %d **\n"), containerTypeAndIndex.m_Index);
            break;
            break;
        }
    }

    {
        const size_t numParams = pblockData.m_ParametersIDsUpdatedInThatParamBlock.length();
        _ftprintf(file, indentString);
        _ftprintf(file, _T("** Num paramIDs in array : %llu **\n"), numParams);

        for(size_t i=0;i<numParams;++i){
            _ftprintf(file, indentStringPlus);
            _ftprintf(file, _T("** ParamID #%llu is : %d **\n"), i, pblockData.m_ParametersIDsUpdatedInThatParamBlock[i]);
        }
    }

    {
        const size_t numParamNames = pblockData.m_ParametersNames.length();
        _ftprintf(file, indentString);
        _ftprintf(file, _T("** Num param names in array : %llu **\n"), numParamNames);

        for(size_t i=0;i<numParamNames;++i){
            _ftprintf(file, indentStringPlus);
            _ftprintf(file, _T("** Param name #%llu is : \"%s\" **\n"), i, pblockData.m_ParametersNames[i].data());
        }
    }
}
