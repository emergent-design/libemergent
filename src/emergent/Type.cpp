#include "emergent/Type.h"

using namespace std;


namespace emergent
{
	map<string, shared_ptr<TypeBase>> TypeBase::masters;
	mutex TypeBase::cs;
}
