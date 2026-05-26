/* =========================================
 *
 *  FCNPC for open.mp - binary entry point
 *
 * ========================================= */

#include <sdk.hpp>
#include "FCNPC.hpp"

// Called by open.mp when the compiled component is loaded.
COMPONENT_ENTRY_POINT()
{
	return FCNPCComponent::getInstance();
}
