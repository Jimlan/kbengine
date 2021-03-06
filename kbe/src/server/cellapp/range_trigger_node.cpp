/*
This source file is part of KBEngine
For the latest info, see http://www.kbengine.org/

Copyright (c) 2008-2012 KBEngine.

KBEngine is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

KBEngine is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.
 
You should have received a copy of the GNU Lesser General Public License
along with KBEngine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "range_list.hpp"
#include "range_trigger.hpp"
#include "range_trigger_node.hpp"
#include "entity_range_node.hpp"

namespace KBEngine{	


//-------------------------------------------------------------------------------------
RangeTriggerNode::RangeTriggerNode(RangeTrigger* pRangeTrigger, float xz, float y):
RangeNode(NULL),
range_xz_(xz),
range_y_(y),
old_range_xz_(range_xz_),
old_range_y_(range_y_),
pRangeTrigger_(pRangeTrigger)
{
	flags(RANGENODE_FLAG_HIDE);

#ifdef _DEBUG
	descr((boost::format("RangeTriggerNode(origin=%1%->%2%)") % pRangeTrigger_->origin() % pRangeTrigger_->origin()->descr()).str());
#endif

	static_cast<EntityRangeNode*>(pRangeTrigger_->origin())->addWatcherNode(this);
}

//-------------------------------------------------------------------------------------
RangeTriggerNode::~RangeTriggerNode()
{
	static_cast<EntityRangeNode*>(pRangeTrigger_->origin())->delWatcherNode(this);
}

//-------------------------------------------------------------------------------------
void RangeTriggerNode::onParentRemove(RangeNode* pParentNode)
{
	if((flags() & RANGENODE_FLAG_REMOVEING) <= 0)
		pParentNode->pRangeList()->remove(this);
}

//-------------------------------------------------------------------------------------
float RangeTriggerNode::xx()const 
{
	if((flags() & RANGENODE_FLAG_REMOVED) > 0 || pRangeTrigger_ == NULL)
		return -FLT_MAX;

	return pRangeTrigger_->origin()->xx() + range_xz_; 
}

//-------------------------------------------------------------------------------------
float RangeTriggerNode::yy()const 
{
	if((flags() & RANGENODE_FLAG_REMOVED) > 0 || pRangeTrigger_ == NULL)
		return -FLT_MAX;

	return pRangeTrigger_->origin()->yy() + range_y_; 
}

//-------------------------------------------------------------------------------------
float RangeTriggerNode::zz()const 
{
	if((flags() & RANGENODE_FLAG_REMOVED) > 0 || pRangeTrigger_ == NULL)
		return -FLT_MAX;

	return pRangeTrigger_->origin()->zz() + range_xz_; 
}

//-------------------------------------------------------------------------------------
void RangeTriggerNode::onNodePassX(RangeNode* pNode, bool isfront)
{
	if((flags() & RANGENODE_FLAG_REMOVED) <= 0 && pRangeTrigger_)
		pRangeTrigger_->onNodePassX(this, pNode, isfront);
}

//-------------------------------------------------------------------------------------
void RangeTriggerNode::onNodePassY(RangeNode* pNode, bool isfront)
{
	if((flags() & RANGENODE_FLAG_REMOVED) <= 0 && pRangeTrigger_)
		pRangeTrigger_->onNodePassY(this, pNode, isfront);
}

//-------------------------------------------------------------------------------------
void RangeTriggerNode::onNodePassZ(RangeNode* pNode, bool isfront)
{
	if((flags() & RANGENODE_FLAG_REMOVED) <= 0 && pRangeTrigger_)
		pRangeTrigger_->onNodePassZ(this, pNode, isfront);
}

//-------------------------------------------------------------------------------------
}
