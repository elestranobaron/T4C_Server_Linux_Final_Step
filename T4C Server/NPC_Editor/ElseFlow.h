#ifndef __ElseFlow__
#define __ElseFlow__

// Include files
#include "ControlFlow.h"
#include "../Core/WDAFile.h"

//  todo
namespace NPC_Editor
{
class IfFlow;
class ElseFlow	: public ControlFlow
{
public:
    friend IfFlow;

	ElseFlow();
	~ElseFlow();
    
    void SaveImp( WDAFile &file );
    void LoadImp( WDAFile &file );

protected:
    Instruction *Clone();
private:
    void UpdateName(){};

};

// END CLASS DEFINITION ElseFlow
            
}
#endif // __ElseFlow__