/*
 Copyright (c) 2008-2022, Benoit AUTHEMAN All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the author or Destrat.io nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL AUTHOR BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

//-----------------------------------------------------------------------------
// This file is a part of the QuickQanava software library.
//
// \file	qanEdgeDraggableCtrl.cpp
// \author	benoit@destrat.io
// \date	2021 10 29
//-----------------------------------------------------------------------------

// QuickQanava headers
#include "./qanEdgeDraggableCtrl.h"
#include "./qanEdgeItem.h"
#include "./qanGraph.h"

namespace qan { // ::qan

qan::Graph*  EdgeDraggableCtrl::getGraph() noexcept
{
    return _targetItem ? _targetItem->getGraph() : nullptr;
}

/* Drag'nDrop Management *///--------------------------------------------------
bool    EdgeDraggableCtrl::handleDragEnterEvent(QDragEnterEvent* event) { Q_UNUSED(event); return false; }

void	EdgeDraggableCtrl::handleDragMoveEvent(QDragMoveEvent* event) { Q_UNUSED(event) }

void	EdgeDraggableCtrl::handleDragLeaveEvent(QDragLeaveEvent* event) { Q_UNUSED(event) }

void    EdgeDraggableCtrl::handleDropEvent(QDropEvent* event) { Q_UNUSED(event) }

void    EdgeDraggableCtrl::handleMouseDoubleClickEvent(QMouseEvent* event) { Q_UNUSED(event) }

bool    EdgeDraggableCtrl::handleMouseMoveEvent(QMouseEvent* event)
{
    // PRECONDITIONS:
        // graph must be non nullptr (configured).
        // _targetItem can't be nullptr
    if (!_targetItem)
        return false;
    const auto graph = _targetItem->getGraph();
    if (graph == nullptr)
        return false;

    // Early exits
    if (event->buttons().testFlag(Qt::NoButton))
        return false;
    if (!_targetItem->getDraggable())   // Do not drag if edge is non draggable
        return false;

    // Do not drag if edge source and destination nodes are locked.
    auto src = _targetItem->getSourceItem() != nullptr ? _targetItem->getSourceItem()->getNode() :
                                                         nullptr;
    auto dst = _targetItem->getDestinationItem() != nullptr ? _targetItem->getDestinationItem()->getNode() :
                                                              nullptr;
    if ((src && src->getLocked()) ||
        (src && src->getIsProtected()) ||
        (dst && dst->getLocked()) ||
        (dst && dst->getIsProtected()))
        return false;

    const auto rootItem = graph->getContainerItem();
    if (rootItem != nullptr &&      // Root item exist, left button is pressed and the target item
        event->buttons().testFlag(Qt::LeftButton)) {    // is draggable and not collapsed
        const auto sceneDragPos = rootItem->mapFromGlobal(event->globalPos());
        if (!_targetItem->getDragged()) {
            beginDragMove(sceneDragPos, false);  // false = no selection (no edge selection support)
            return true;
        } else {
            dragMove(sceneDragPos, false);  // false = no selection (no edge selection support)
            return true;
        }
    }
    return false;
}

void    EdgeDraggableCtrl::handleMousePressEvent(QMouseEvent* event) { Q_UNUSED(event) }

void    EdgeDraggableCtrl::handleMouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
    if (_targetItem &&
        _targetItem->getDragged())
        endDragMove();
}

void    EdgeDraggableCtrl::beginDragMove(const QPointF& sceneDragPos, bool dragSelection)
{
    Q_UNUSED(dragSelection)
    if (!_targetItem)
        return;

    _targetItem->setDragged(true);
    _initialDragPos = sceneDragPos;
    const auto rootItem = getGraph()->getContainerItem();
    if (rootItem != nullptr)   // Project in scene rect (for example is a node is part of a group)
        _initialTargetPos = rootItem->mapFromItem(_targetItem, QPointF{0,0});

    // Get target edge adjacent nodes
    auto src = _targetItem->getSourceItem() != nullptr &&
               _targetItem->getSourceItem()->getNode() != nullptr ? _targetItem->getSourceItem()->getNode()->getItem() :
                                                                    nullptr;
    auto dst = _targetItem->getDestinationItem() != nullptr &&
               _targetItem->getDestinationItem()->getNode() != nullptr ? _targetItem->getDestinationItem()->getNode()->getItem() :
                                                                         nullptr;
    if (src != nullptr)
        src->draggableCtrl().beginDragMove(sceneDragPos, false);
    if (dst != nullptr)
        dst->draggableCtrl().beginDragMove(sceneDragPos, false);
}

void    EdgeDraggableCtrl::dragMove(const QPointF& sceneDragPos, bool dragSelection,
                                    bool disableSnapToGrid, bool disableOrientation)
{
    Q_UNUSED(dragSelection)
    Q_UNUSED(disableSnapToGrid)
    Q_UNUSED(disableOrientation)
    // PRECONDITIONS:
        // _targetItem must be configured
    if (!_targetItem)
        return;
    if (_targetItem->getSourceItem() == nullptr ||
        _targetItem->getDestinationItem() == nullptr)
                return;
    // Get target edge adjacent nodes
    auto src = _targetItem->getSourceItem()->getNode() != nullptr ? _targetItem->getSourceItem()->getNode()->getItem() :
                                                                    nullptr;
    auto dst = _targetItem->getDestinationItem()->getNode() != nullptr ? _targetItem->getDestinationItem()->getNode()->getItem() :
                                                                         nullptr;
    // Polish snapToGrid:
    // When edge src|dst is not vertically or horizontally aligned: disable hook.
    // If they are vertically/horizontally aligned: allow move snapToGrid it won't generate jiterring.
    const auto disableHooksSnapToGrid = _targetItem->getSourceItem()->getDragOrientation() == qan::NodeItem::DragOrientation::DragAll ||
                                        _targetItem->getSourceItem()->getDragOrientation() == qan::NodeItem::DragOrientation::DragAll ||
                                        (_targetItem->getSourceItem()->getDragOrientation() != _targetItem->getDestinationItem()->getDragOrientation());
    const auto disableHooksDragOrientation = (_targetItem->getSourceItem()->getDragOrientation() == qan::NodeItem::DragOrientation::DragHorizontal ||
                                              _targetItem->getSourceItem()->getDragOrientation() == qan::NodeItem::DragOrientation::DragVertical) &&
                                             (_targetItem->getSourceItem()->getDragOrientation() == _targetItem->getDestinationItem()->getDragOrientation());
    if (src != nullptr)
        src->draggableCtrl().dragMove(sceneDragPos, /*dragSelection=*/false,
                                      /*disableSnapToGrid=*/disableHooksSnapToGrid, disableHooksDragOrientation);
    if (dst != nullptr)
        dst->draggableCtrl().dragMove(sceneDragPos, /*dragSelection=*/false,
                                      /*disableSnapToGrid=*/disableHooksSnapToGrid, disableHooksDragOrientation);
}

void    EdgeDraggableCtrl::endDragMove(bool dragSelection)
{
    Q_UNUSED(dragSelection)
    if (!_targetItem)
        return;

    // FIXME #141 add an edgeMoved() signal...
    //emit graph->nodeMoved(_target);
    _targetItem->setDragged(false);

    // Get target edge adjacent nodes
    auto src = _targetItem->getSourceItem() != nullptr &&
               _targetItem->getSourceItem()->getNode() != nullptr ? _targetItem->getSourceItem()->getNode()->getItem() :
                                                                    nullptr;
    auto dst = _targetItem->getDestinationItem() != nullptr &&
               _targetItem->getDestinationItem()->getNode() != nullptr ? _targetItem->getDestinationItem()->getNode()->getItem() :
                                                                         nullptr;
    if (src != nullptr)
        src->draggableCtrl().endDragMove(false);
    if (dst != nullptr)
        dst->draggableCtrl().endDragMove(false);
}
//-----------------------------------------------------------------------------

} // ::qan
