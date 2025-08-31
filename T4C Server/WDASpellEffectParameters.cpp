#include "stdafx.h"
#include "WDASpellEffectParameters.h"
#include "Format.h"

using namespace std;
using namespace vir;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

WDASpellEffectParameters::WDASpellEffectParameters( Logger &cLogger, DEBUG_LEVEL dlMapDebugHigh ) : WDATable( cLogger )
{
    MapDebugHighLogLevel( dlMapDebugHigh );
}

WDASpellEffectParameters::~WDASpellEffectParameters()
{

}

// Accessors
//////////////////////////////////////////////////////////////////////////////////////////
string &WDASpellEffectParameters::GetParamValue( void )
//////////////////////////////////////////////////////////////////////////////////////////
//  Returns the paramter value
//////////////////////////////////////////////////////////////////////////////////////////
{
    return csParamValue;
}

//////////////////////////////////////////////////////////////////////////////////////////
DWORD WDASpellEffectParameters::GetParamID( void )
//////////////////////////////////////////////////////////////////////////////////////////
//  Returns the parameterID.
//////////////////////////////////////////////////////////////////////////////////////////
{
    return dwParamID;
}

// Loading/Saving     
//////////////////////////////////////////////////////////////////////////////////////////
void WDASpellEffectParameters::SaveTo
//////////////////////////////////////////////////////////////////////////////////////////
// Saves to a wdaFile
// 
(
 WDAFile &wdaFile // the wdaFile
)
//////////////////////////////////////////////////////////////////////////////////////////
{
    // Save the parameter ID
    wdaFile.Write( dwParamID );

    // Save the parameter value.
    wdaFile.Write( csParamValue );

    TFormat cFormat;
    cOutput.Log(
        dlDebugHigh,
        cFormat(
            "Wrote Param( %u, %s ) ",
            dwParamID,
            csParamValue.c_str()
        )
    );

}
    
//////////////////////////////////////////////////////////////////////////////////////////
void WDASpellEffectParameters::CreateFrom
//////////////////////////////////////////////////////////////////////////////////////////
// Creates from a wdaFile.
// 
(
 WDAFile &wdaFile, // the wdaFile
 bool //createReadOnly
)
//////////////////////////////////////////////////////////////////////////////////////////
{
    // Load the parameter ID
    wdaFile.Read( dwParamID );

    // Load the parameter value.
    wdaFile.Read( csParamValue );
    
    TFormat cFormat;
    cOutput.Log(
        dlDebugHigh,
        cFormat(
            "Param( %u, %s ) ",
            dwParamID,
            csParamValue.c_str()
        )
    );

}

//////////////////////////////////////////////////////////////////////////////////////////
bool WDASpellEffectParameters::operator==( const WDASpellEffectParameters &l )
{
    return dwParamID == l.dwParamID && csParamValue == l.csParamValue;
}

#ifndef NO_DAO_SUPPORT    
    
//////////////////////////////////////////////////////////////////////////////////////////
void WDASpellEffectParameters::CreateFrom
//////////////////////////////////////////////////////////////////////////////////////////
// Loads the worlds from a DAO support    
// 
(
 CDaoRecordset &cRecord, // The sole record where the spell parameter resides.
 CDaoDatabase &cDatabase // The database.
)
//////////////////////////////////////////////////////////////////////////////////////////
{
    // If we didn't get the recordset's end.
    ASSERT( !cRecord.IsEOF() );
    if( cRecord.IsEOF() ){
        return;
    }
    COleVariant oleData;
    CString csValue;

    cRecord.GetFieldValue( "ParamID", oleData );
    dwParamID = V_I4( &oleData );

    cRecord.GetFieldValue( "Value", oleData );
    csParamValue = V_BSTRT( &oleData );

    TFormat cFormat;
    cOutput.Log(
        dlDebugHigh,
        cFormat(
            "Param( %u, %s ) ",
            dwParamID,
            csParamValue.c_str()
        )
    );
}
#endif

