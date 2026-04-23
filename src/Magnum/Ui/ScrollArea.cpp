/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025, 2026
              Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include "ScrollArea.h"

#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Containers/Function.h>

#include "Magnum/Ui/AbstractTheme.hpp"
#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/DataLayer.h"
#include "Magnum/Ui/EventLayer.h"
#include "Magnum/Ui/GenericLayouter.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/LayoutLayer.h"
#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/SnapLayout.h"
#include "Magnum/Ui/SnapLayouter.h"
#include "Magnum/Ui/UserInterface.h"

namespace Magnum { namespace Ui {

Debug& operator<<(Debug& debug, const ScrollAreaFlag value) {
    debug << "Ui::ScrollAreaFlag" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case ScrollAreaFlag::value: return debug << "::" #value;
        _c(OnlyX)
        _c(OnlyY)
        _c(NoContentsDrag)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const ScrollAreaFlags value) {
    return Containers::enumSetDebugOutput(debug, value, "Ui::ScrollAreaFlags{}", {
        ScrollAreaFlag::OnlyX,
        ScrollAreaFlag::OnlyY,
        ScrollAreaFlag::NoContentsDrag,
    });
}

using Implementation::BaseStyle;
using Implementation::LayoutStyle;

namespace {

/* Scroll area properties in either horizontal or vertical direction */
struct ScrollAreaStorage: AbstractStorage {
    struct Data {
        /* Width or height of the clipping scroll area view */
        Float viewSize;
        /* Size of the associated scrollbar, for mapping thumb movement to
           contents offset in a way that causes the thumb to be ultimately
           positioned exactly by the offset it was dragged by */
        Float scrollbarSize;
        /* Size of the actual contents. If larger than viewSize, a scrollbar
           thumb is shown in a position appropriate for contents offset, if
           smaller, the thumb is hidden. */
        Float contentsSize = 2;
        /* Offset of the contents inside the view. Is exactly what's passed to
           ui.setNodeOffsetX() / setNodeOffsetY(), thus is also negaitve. */
        Float contentsOffset;
    };

    /* Right now the storage is private to the ScrollArea widget and thus is
       ReferenceCounted to not leak it once the ScrollArea is removed */
    explicit ScrollAreaStorage(DataLayer& layer): AbstractStorage{layer, StorageFlag::ReferenceCounted} {
        /* Zero-initialize the size and offset values. The assumption is that
           the Data is sufficiently small to fit in-place even on 32-bit. If it
           wouldn't, this won't compile. */
        *createInPlace<Data>() = {};
    }

    /* Called from ui.genericLayouter() on each layout update, refreshes
       internal state to match current UI node sizes, marks the storage as
       dirty if it changes, and returns current thumb offset and size. The min
       thumb size should be coming from the thumb node layout properties. */
    Containers::Pair<Float, Float> update(const Float viewSize, const Float scrollbarSize, const Float contentsSize, /*mutable*/ Float contentsOffset, const Float minThumbSize) {
        /* Expect that nobody fiddled with the offset from outside, making it
           larger than 0 (i.e., moved before the begin). OTOH, it can overflow
           on the other side, for example if the view was scrolled to the end
           and the view then got larger due to window resize (or vice versa, if
           the view stays the same but the contents shrink). Clamp the offset
           in that case so the storage has it correct. */
        CORRADE_INTERNAL_ASSERT(contentsOffset <= 0.0f);
        contentsOffset = Math::max(contentsOffset, Math::min(viewSize - contentsSize, 0.0f));

        /* The thumb size is ratio of the contents and view size applied to the
           actual scrollbar length, but at least the min size so it doesn't get
           too small to aim for. On the other hand, if the contents are not
           larger than the view, the thumb is made zero-size and thus
           invisible, effectively disabling scrolling. */
        const Float thumbSize = contentsSize <= viewSize ? 0.0f :
            Math::max(scrollbarSize*viewSize/contentsSize, minThumbSize);
        /* The offset is then ratio of the contents offset to size of the
           contents outside of the visible view (i.e., 0% is when the contents
           top/left edge is at the view top/left edge but 100% is when the
           contents bottom/right edge is at the view bottom/right edge) applied
           to the remaining length of the scrollbar where the thumb can move.
           The contents node offset is negative, while thumb offset is
           positive, so it's divided by a negative value to flip its sign.
           Again, if contents size is not larger than view size, the thumb is
           put at zero offset, as otherwise the offset could become a NaN. */
        const Float thumbOffset = contentsSize <= viewSize ? 0.0f :
            (scrollbarSize - thumbSize)*contentsOffset/(viewSize - contentsSize);

        /* Note that we're *not* calling setDirty() if the offset changes as
           a result of this update, such as when it got clamped above. The
           storage being dirty results in setNodeOffset() to be called on the
           UI, but this update function is called during a layout step, at
           which point it's too late for anything to pick up the change to the
           node offset. Best case it'd result in the change being applied next
           frame, which is still too late and would result in weird hiccups.
           Instead, the same clamp operation (and the same assert) is done via
           another generic layout, which ensures that the actual offset used
           by the layout is in bounds as well. Then, once the view gets
           scrolled via an event, setDirty() called from there will turn the
           original node offset in the UI and the layout node offset back in
           sync. */
        /** @todo may want to actually mark as dirty if the storage ever gets
            public and user code can attach to it being dirty, but for that
            need to first fix that the LayerState gets actually preserved
            beyond the UI update (right now, if setDirty() is called here, the
            storage stays marked as dirty, but the LayerState is reset back to
            empty as this whole process happens inside layer update which
            clears LayerState at the end */

        Data& data = *AbstractStorage::data<Data>();
        data.viewSize = viewSize;
        data.scrollbarSize = scrollbarSize;
        data.contentsSize = contentsSize;
        data.contentsOffset = contentsOffset;
        return {thumbOffset, thumbSize};
    }

    /* When this changes, the storage is marked as dirty, and setNodeOffset*()
       gets called on the contents node */
    StorageQuery<Float> contentsOffset() const {
        return {*this, {}, [](const ScrollAreaStorage& storage, StorageOperation) {
            return storage.data<Data>()->contentsOffset;
        }};
    }

    /* When this changes, the storage is *not* marked as dirty, as it currently
       isn't used to update anything in the UI, is only a getter for
       diagnostics */
    Float contentsPercentage() const {
        const Data& data = *AbstractStorage::data<Data>();

        /* If the contents are not larger than the view or update() wasn't
           called yet (which could thus result in division by zero below),
           we're not scrolled anywhere */
        if(!data.viewSize || data.contentsSize <= data.viewSize) {
            CORRADE_INTERNAL_ASSERT(!data.contentsOffset);
            return 0.0f;
        }

        /* The offset is stored negative to match what's sent to
           setNodeOffset*(), turn it into a ratio to size of the contents
           outside of the visible view, negative to flip the sign */
        return 100.0f*data.contentsOffset/(data.viewSize - data.contentsSize);
    }

    /* Scroll to a particular percentage, called from
       ScrollArea::scrollToPercentage*() */
    /** @todo these are const because otherwise capturing a ScrollAreaStorage
        instance inside a lambda makes it impossible to call anything on it,
        figure out if that's alright of if there's a better usage pattern */
    void scrollToPercentage(Float percentage) const {
        Data& data = *AbstractStorage::data<Data>();

        /* If the contents are not larger than the view or update() wasn't
           called yet (which would lead to scroll percentage out of the
           [0, 100] range), exit without doing anything */
        if(!data.viewSize || data.contentsSize <= data.viewSize) {
            CORRADE_INTERNAL_ASSERT(!data.contentsOffset);
            return;
        }

        /* Turn the percentage into a clamped ratio and apply it to the size of
           the contents outside of the visible view. The offset is negative, so
           multiply the percentage by a negative value to flip the sign. */
        const Float contentsOffset = (data.viewSize - data.contentsSize)*Math::clamp(percentage*0.01f, 0.0f, 1.0f);

        /* Mark the storage as dirty only if the offset actually changes,
           e.g. scrolling to exactly the current position won't do
           anything */
        if(Math::notEqual(data.contentsOffset, contentsOffset)) {
            data.contentsOffset = contentsOffset;
            setDirty();
        }
    }

    /* Scroll the view by given delta, i.e. the delta being interpreted
       directly as the value to add to the node offset */
    /** @todo these are const because otherwise capturing a ScrollAreaStorage
        instance inside a lambda makes it impossible to call anything on it,
        figure out if that's alright of if there's a better usage pattern */
    void scrollViewBy(Float delta) const {
        Data& data = *AbstractStorage::data<Data>();

        /* In practice this function should always be called with the UI
           already laid out, so with update() called */
        CORRADE_INTERNAL_ASSERT(data.viewSize);

        /* The offset is negative, so a positive delta decreases it. Clamp it
           so it's between 0 and (negative) size of the contents outside of the
           visible view. If contents are not larger than the view, the
           resulting offset should be zero, as otherwise it'd lead to scroll
           percentage out of the [0, 100] range. */
        const Float contentsOffset = Math::clamp(data.contentsOffset + delta, data.viewSize - data.contentsSize, 0.0f);
        CORRADE_INTERNAL_ASSERT(data.contentsSize > data.viewSize || !contentsOffset);

        /* Mark the storage as dirty only if the offset actually changes, e.g.
           scrolling forward at the end won't do anything */
        if(Math::notEqual(data.contentsOffset, contentsOffset)) {
            data.contentsOffset = contentsOffset;
            setDirty();
        }
    }

    /* Scroll the thumb by given delta, i.e. resulting in the thumb (not the
       view) ultimately moving by given delta, thus correctly following the
       pointer */
    /** @todo these are const because otherwise capturing a ScrollAreaStorage
        instance inside a lambda makes it impossible to call anything on it,
        figure out if that's alright of if there's a better usage pattern */
    void scrollThumbBy(Float delta) const {
        Data& data = *AbstractStorage::data<Data>();

        /* Scroll by the ratio of the delta to size of the scrollbar applied to
           whole size of the contents, but interpreting the delta in the other
           direction (because while dragging the view forward scrolls backward,
           dragging the thumb forward scrolls forward). The scrollViewBy()
           function then takes care of all clamping, marking as dirty etc. */
        scrollViewBy(-data.contentsSize*delta/data.scrollbarSize);
    }
};

/* Calculates a positive/negative offset for scrolling on the X/Y scrollbar,
   where both horizontal and vertical wheel as well as 2D scroll on the
   touchpad should work.

   Based on a quick investigation of how e.g. Qt does this, it seems that the
   *length* of the vector is taken, and everything above 45° / 225° results in
   scroll left/up, and everything below bottom/right. The X and Y signs should
   match the usual drag behavior, i.e. dragging bottom/right meaning *adding*
   to the node offset thus positive, and up/left subtracting and thus negative:

               |     / 45°
          up/left  /
        +X +Y  | /-X +Y
        -------+-------
        +X -Y/ |  -X -Y
           / bottom/right
    225° /     |

   The `offset` is pointing to bottom/right if both axes are positive and to
   top/left if both negative. For the remaining two quadrants, the 45°
   edge is when `-offset.x() == offset.y()`. Put together, the condition
   `-offset.x() > offset.y()` is what distinguishes bottom/right (and thus
   negative offset) from up/left (and positive offset). */
Float directionalScrollOffset(const Vector2& offset) {
    return offset.length()*(-offset.x() > offset.y() ? -1.0f : 1.0f);
}

}

ScrollArea::ScrollArea(const Anchor anchor, const ScrollAreaFlags flags): Widget{anchor}, _flags{flags} {
    /* *Technically* this could work, just be useless, but there would have to
       be one more LayoutStyle variant for something that likely nobody will
       ever use, so disallow it */
    CORRADE_ASSERT(!(flags >= (ScrollAreaFlag::OnlyX|ScrollAreaFlag::OnlyY)),
        "Ui::ScrollArea:" << ScrollAreaFlag::OnlyX << "and" << ScrollAreaFlag::OnlyY << "are mutually exclusive", );

    /* Turn this node into a layout, if it's not already. The LayoutStyle
       specifies outer margin, min sizes should get propagated from
       scrollbars. */
    SnapLayout layout{anchor};
    ui().layoutLayer().create(LayoutStyle::ScrollArea, anchor);

    /* Create the three inner nodes directly and then order them so the view is
       with the lowest ID, so in case the scrollbars are placed over the
       contents and not beside them, the contents are drawn underneath and have
       events processed last. */
    _viewNode = ui().createNode(layout, {}, {});
    _scrollbarXNode = flags >= ScrollAreaFlag::OnlyY ?
        NodeHandle::Null : ui().createNode(layout, {}, {});
    _scrollbarYNode = flags >= ScrollAreaFlag::OnlyX ?
        NodeHandle::Null : ui().createNode(layout, {}, {});
    if(_scrollbarXNode != NodeHandle::Null && nodeHandleId(_viewNode) > nodeHandleId(scrollbarXNode()))
        Utility::swap(_viewNode, _scrollbarXNode);
    if(_scrollbarYNode != NodeHandle::Null && nodeHandleId(_viewNode) > nodeHandleId(scrollbarYNode()))
        Utility::swap(_viewNode, _scrollbarYNode);
    /* Just two comparisons are apparently enough, don't need to deal with
       mutually ordering the scrollbar nodes. Verified extensively in
       ScrollAreaTest::scrollbarEventOrder(). */
    CORRADE_INTERNAL_ASSERT(_scrollbarXNode == NodeHandle::Null || nodeHandleId(_viewNode) < nodeHandleId(_scrollbarXNode));
    CORRADE_INTERNAL_ASSERT(_scrollbarYNode == NodeHandle::Null || nodeHandleId(_viewNode) < nodeHandleId(_scrollbarYNode));

    /* View node, which clips the contents and has fallthrough enabled for
       drag-to-scroll unless NoContentsDrag is set. Depending on style the
       associated LayoutStyle may give it margin on appropriate sides for the
       scrollbars to fit in, or the scrollbars get overlaid on top. In the
       direction where it doesn't scroll it expands based on contents as
       appropriate. */
    ui().addNodeFlags(_viewNode, NodeFlag::Clip|(flags >= ScrollAreaFlag::NoContentsDrag ?
        NodeFlags{} : NodeFlag::FallthroughPointerEvents));
    SnapLayout viewLayout{ui(), ui().snapLayouter().addExplicit(
        _viewNode, Snap::Fill, layout,
        (flags >= ScrollAreaFlag::OnlyX ?
            SnapLayoutFlags{} : SnapLayoutFlag::IgnoreOverflowY)|
        (flags >= ScrollAreaFlag::OnlyY ?
            SnapLayoutFlags{} : SnapLayoutFlag::IgnoreOverflowX))};
    {
        LayoutStyle style;
        if(!(flags & (ScrollAreaFlag::OnlyX|ScrollAreaFlag::OnlyY)))
            style = LayoutStyle::ScrollAreaView;
        else if(flags >= ScrollAreaFlag::OnlyX)
            style = LayoutStyle::ScrollAreaViewOnlyX;
        else if(flags >= ScrollAreaFlag::OnlyY)
            style = LayoutStyle::ScrollAreaViewOnlyY;
        else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
        ui().layoutLayer().create(style, _viewNode);
    }

    /* The actual (overflowing) scroll area contents are a child of the inner
       node. It's exposed via contents() as a SnapLayout to the user and for
       correct behavior the layout shouldn't add any offset to the node, thus
       we're snapping to TopLeft. If scrolling only in one direction, the other
       is made to fill the view contents. */
    _contentsNode = viewLayout.snapChild(Snap::TopLeft|
        (flags >= ScrollAreaFlag::OnlyY ? Snap::FillX : Snaps{})|
        (flags >= ScrollAreaFlag::OnlyX ? Snap::FillY : Snaps{}));

    /* Horizontal scrolling, unless disabled */
    if(!(flags >= ScrollAreaFlag::OnlyY)) {
        /* Layout for the scrollbar node, snapped to the whole right edge. The
           LayoutStyle supplies size constraints and margins, BaseStyle the
           actual visuals. */
        SnapLayout scrollbarXLayout{ui(), ui().snapLayouter().addExplicit(
            _scrollbarXNode, Snap::Bottom|Snap::FillX, layout)};
        ui().layoutLayer().create(flags >= ScrollAreaFlag::OnlyX ?
            LayoutStyle::ScrollbarOnlyX : LayoutStyle::ScrollbarX, _scrollbarXNode);
        ui().baseLayer().create(BaseStyle::ScrollbarX, _scrollbarXNode);

        /* Scrollbar thumb. The LayoutStyle defines its minimal width, other
           than that it's set to fill the scrollbar horizontally. */
        _scrollbarThumbXNode = scrollbarXLayout.snapChild(Snap::FillY|Snap::Left);
        ui().layoutLayer().create(LayoutStyle::ScrollbarThumbX, _scrollbarThumbXNode);
        ui().baseLayer().create(BaseStyle::ScrollbarThumbX, _scrollbarThumbXNode);

        /* The storage persists the view size and the contents offset + size.
           The layouter updates the storage with actual sizes and offsets, and
           subsequently uses the storage values to position and size the thumb.

           If the offset differs from what was there before (for example if the
           view or the contents size is different, causing the offset to be
           clamped), the storage marks itself as dirty, which then results in
           setNodeOffsetY() being called below. Note that if the node or
           contents size differs, it *doesn't* mark the storage as dirty, as
           that doesn't affect anything besides the thumb, which is already
           up-to-date. */
        ScrollAreaStorage storage{ui().dataLayer()};
        _scrollXStorage = storageHandleStorage(storage.handle());
        {
            /** @todo clean this up once I can use C++14 named captures */
            const NodeHandle viewNode = _viewNode;
            const NodeHandle contentsNode = _contentsNode;
            const NodeHandle scrollbarXNode = _scrollbarXNode;
            DataLayer& dataLayer = ui().dataLayer();
            const DataLayerStorageHandle scrollXStorage = _scrollXStorage;
            /* The storage is passed as a pointer + 4B handle instead of the
               16B ScrollAreaStorage instance to avoid capture allocation on
               64-bit */
            /** @todo could be non-allocated on 32-bit as well if the UI
                (and thus the data layer) could be accessed through an
                argument or through the layouter somehow, turning the
                capture into just 16 bytes instead of 20 */
            ui().genericLayouter().add(_scrollbarThumbXNode, [&dataLayer, scrollXStorage, contentsNode, viewNode, scrollbarXNode](const GenericLayouter& layouter, const NodeHandle node, Vector2& nodeOffset, Vector2& nodeSize)
            {
                const Containers::Pair<Float, Float> thumbOfsetSize = dataLayer.storage<ScrollAreaStorage>(scrollXStorage).update(
                    layouter.nodeSize(viewNode).x(),
                    layouter.nodeSize(scrollbarXNode).x(),
                    layouter.nodeSize(contentsNode).x(),
                    layouter.nodeOffset(contentsNode).x(),
                    layouter.nodeMinSize(node).x());

                nodeOffset.x() = thumbOfsetSize.first();
                nodeSize.x() = thumbOfsetSize.second();
            });
        } {
            /** @todo clean this up once I can use C++14 named captures */
            const NodeHandle contentsNode = _contentsNode;
            UserInterface& ui = this->ui();
            ui.dataLayer().onUpdate(storage.contentsOffset(), [&ui, contentsNode](const Float& offset) {
                /** @todo this gets called even for the very first update()
                    because that's what the DataLayer implicitly does for all
                    newly created data bindings, it's however unnecessary as
                    the offset is propagated from the node offset in the first
                    place -- add some DataBindingFlag for this? it's also
                    rather hard to auto-test because there's no observable
                    difference */
                ui.setNodeOffsetX(contentsNode, offset);
            }, _contentsNode);
        }

        /* Dragging the thumb / scrolling the scrollbar updates the storage
           which leads to setNodeOffsetX() being called. Want exactly the same
           scroll response as with onDragOrScroll() below, so multiply the
           wheel offset by the same value as is used by onDragOrScroll(). */
        ui().eventLayer().onDrag(_scrollbarThumbXNode, [storage](const Vector2& offset) {
            storage.scrollThumbBy(offset.x());
        });
        {
            /** @todo clean this up once I can use C++14 named captures */
            EventLayer& eventLayer = ui().eventLayer();
            ui().eventLayer().onScroll(_scrollbarXNode, [&eventLayer, storage](const Vector2& offset) {
                storage.scrollViewBy(directionalScrollOffset(offset*eventLayer.scrollStepDistance()));
            });
        }
    } else {
        _scrollbarThumbXNode = NodeHandle::Null;
        _scrollXStorage = DataLayerStorageHandle::Null;
    }

    /* Vertical scrolling, unless disabled. Basically just a copy of the above
       but in the other direction. */
    if(!(flags >= ScrollAreaFlag::OnlyX)) {
        /* Layout for the scrollbar node, snapped to the whole right edge */
        SnapLayout scrollbarLayout{ui(), ui().snapLayouter().addExplicit(
            _scrollbarYNode, Snap::Right|Snap::FillY, layout)};
        ui().layoutLayer().create(flags >= ScrollAreaFlag::OnlyY ?
            LayoutStyle::ScrollbarOnlyY : LayoutStyle::ScrollbarY, _scrollbarYNode);
        ui().baseLayer().create(BaseStyle::ScrollbarY, _scrollbarYNode);

        /* Scrollbar thumb. The LayoutStyle defines its minimal height, other
           than that it's set to fill the scrollbar horizontally. */
        _scrollbarThumbYNode = scrollbarLayout.snapChild(Snap::FillX|Snap::Top);
        ui().layoutLayer().create(LayoutStyle::ScrollbarThumbY, _scrollbarThumbYNode);
        ui().baseLayer().create(BaseStyle::ScrollbarThumbY, _scrollbarThumbYNode);

        /* Storage persisting the horizontal scroll area properties */
        ScrollAreaStorage storage{ui().dataLayer()};
        _scrollYStorage = storageHandleStorage(storage.handle());
        {
            /** @todo clean this up once I can use C++14 named captures */
            const NodeHandle viewNode = _viewNode;
            const NodeHandle contentsNode = _contentsNode;
            const NodeHandle scrollbarYNode = _scrollbarYNode;
            DataLayer& dataLayer = ui().dataLayer();
            const DataLayerStorageHandle scrollYStorage = _scrollYStorage;
            /** @todo could be non-allocated on 32-bit as well, see above */
            ui().genericLayouter().add(_scrollbarThumbYNode, [&dataLayer, scrollYStorage, contentsNode, viewNode, scrollbarYNode](const GenericLayouter& layouter, NodeHandle node, Vector2& nodeOffset, Vector2& nodeSize) {
                const Containers::Pair<Float, Float> thumbOfsetSize = dataLayer.storage<ScrollAreaStorage>(scrollYStorage).update(
                    layouter.nodeSize(viewNode).y(),
                    layouter.nodeSize(scrollbarYNode).y(),
                    layouter.nodeSize(contentsNode).y(),
                    layouter.nodeOffset(contentsNode).y(),
                    layouter.nodeMinSize(node).y());

                nodeOffset.y() = thumbOfsetSize.first();
                nodeSize.y() = thumbOfsetSize.second();
            });
        } {
            /** @todo clean this up once I can use C++14 named captures */
            const NodeHandle contentsNode = _contentsNode;
            UserInterface& ui = this->ui();
            ui.dataLayer().onUpdate(storage.contentsOffset(), [&ui, contentsNode](const Float& offset) {
                /** @todo this gets called even for the very first update(),
                    same as with the X case above */
                ui.setNodeOffsetY(contentsNode, offset);
            }, _contentsNode);
        }

        /* Dragging the thumb / scrolling the scrollbar */
        ui().eventLayer().onDrag(_scrollbarThumbYNode, [storage](const Vector2& offset) {
            storage.scrollThumbBy(offset.y());
        });
        {
            /** @todo clean this up once I can use C++14 named captures */
            EventLayer& eventLayer = ui().eventLayer();
            ui().eventLayer().onScroll(_scrollbarYNode, [&eventLayer, storage](const Vector2& offset) {
                storage.scrollViewBy(directionalScrollOffset(offset*eventLayer.scrollStepDistance()));
            });
        }
    } else {
        _scrollbarThumbYNode = NodeHandle::Null;
        _scrollYStorage = DataLayerStorageHandle::Null;
    }

    /* Scrolling or dragging the contents updates the storage, which leads
       to setNodeOffsetY() being called. The view has FallthroughPointerEvents
       enabled, which means dragging will work even with child nodes present. */
    /** @todo because currently the EventLayer associates the fallthrough with
        a data and not a node, there has to be exactly one onDragOrScroll()
        handler that handles both directions, with two of them it'd conflict
        and not work at all. With just onScroll() this isn't an issue, but it's
        still nicer to have just one event handler attached for both directions
        instead of two. */
    if(!(flags & (ScrollAreaFlag::OnlyX|ScrollAreaFlag::OnlyY))) {
        /** @todo clean this up once I can use C++14 named captures */
        const DataLayerStorageHandle scrollXStorage = _scrollXStorage;
        const DataLayerStorageHandle scrollYStorage = _scrollYStorage;
        /* If drag is disabled, we react just to scroll, and have to multiply
           by the step distance again */
        if(flags >= ScrollAreaFlag::NoContentsDrag) {
            /* Passing the UI because dataLayer and eventLayer together with
               two storage handles make the capture allocated on 32-bit. It's
               fine in the other cases below. */
            /** @todo clean this up once I can use C++14 named captures */
            UserInterface& ui = this->ui();
            ui.eventLayer().onScroll(_viewNode, [&ui, scrollXStorage, scrollYStorage](const Vector2& offset) {
                ui.dataLayer().storage<ScrollAreaStorage>(scrollXStorage).scrollViewBy(offset.x()*ui.eventLayer().scrollStepDistance().x());
                ui.dataLayer().storage<ScrollAreaStorage>(scrollYStorage).scrollViewBy(offset.y()*ui.eventLayer().scrollStepDistance().y());
            });
        } else {
            /** @todo clean this up once I can use C++14 named captures */
            DataLayer& dataLayer = ui().dataLayer();
            ui().eventLayer().onDragOrScroll(_viewNode, [&dataLayer, scrollXStorage, scrollYStorage](const Vector2& offset) {
                dataLayer.storage<ScrollAreaStorage>(scrollXStorage).scrollViewBy(offset.x());
                dataLayer.storage<ScrollAreaStorage>(scrollYStorage).scrollViewBy(offset.y());
            });
        }
    } else if(flags >= ScrollAreaFlag::OnlyX) {
        /** @todo clean this up once I can use C++14 named captures */
        DataLayer& dataLayer = ui().dataLayer();
        EventLayer& eventLayer = ui().eventLayer();
        const DataLayerStorageHandle scrollXStorage = _scrollXStorage;
        /* If drag is disabled, we react just to scroll, and have to multiply
           by the step distance again */
        if(flags >= ScrollAreaFlag::NoContentsDrag)
            ui().eventLayer().onScroll(_viewNode, [&dataLayer, &eventLayer, scrollXStorage](const Vector2& offset) {
                dataLayer.storage<ScrollAreaStorage>(scrollXStorage).scrollViewBy(offset.x()*eventLayer.scrollStepDistance().x());
            });
        else
            ui().eventLayer().onDragOrScroll(_viewNode, [&dataLayer, scrollXStorage](const Vector2& offset) {
                dataLayer.storage<ScrollAreaStorage>(scrollXStorage).scrollViewBy(offset.x());
            });
    } else if(flags >= ScrollAreaFlag::OnlyY) {
        /** @todo clean this up once I can use C++14 named captures */
        DataLayer& dataLayer = ui().dataLayer();
        EventLayer& eventLayer = ui().eventLayer();
        const DataLayerStorageHandle scrollYStorage = _scrollYStorage;
        /* If drag is disabled, we react just to scroll, and have to multiply
           by the step distance again */
        if(flags >= ScrollAreaFlag::NoContentsDrag)
            ui().eventLayer().onScroll(_viewNode, [&dataLayer, &eventLayer, scrollYStorage](const Vector2& offset) {
                dataLayer.storage<ScrollAreaStorage>(scrollYStorage).scrollViewBy(offset.y()*eventLayer.scrollStepDistance().y());
            });
        else
            ui().eventLayer().onDragOrScroll(_viewNode, [&dataLayer, scrollYStorage](const Vector2& offset) {
                dataLayer.storage<ScrollAreaStorage>(scrollYStorage).scrollViewBy(offset.y());
            });
    } else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */

    /* Make sure the scrolled offset doesn't go out of bounds, for example if
       a small view got scrolled at the end and then is resized to large enough
       that no scrolling is needed. The same clamp (and assert) is done in
       ScrollAreaStorage::update() above, but as that function is called during
       layout of the X/Y thumb, it cannot affect layout of the view (and it's
       too late to call setDirty() there to have it propagated to the view
       node), it has to be done independently here as well. The original and
       post-layout node offset will get back in sync once the view is scrolled
       again.

       This layout-side clamping also has an effect that resizing to a larger
       view and then back restores the exact original scroll position, without
       shifting the view somewhere else entirely. Which is nice UX I think. */
    {
        /** @todo clean this up once I can use C++14 named captures */
        const NodeHandle viewNode = _viewNode;
        ui().genericLayouter().add(_contentsNode, [viewNode](const GenericLayouter& layouter, Vector2& nodeOffset, Vector2& nodeSize) {
            CORRADE_INTERNAL_ASSERT(nodeOffset <= Vector2{});
            nodeOffset = Math::max(nodeOffset, Math::min(layouter.nodeSize(viewNode) - nodeSize, Vector2{}));
        });
    }
}

ScrollArea::ScrollArea(NonOwnedT, const Anchor anchor, const ScrollAreaFlags flags): ScrollArea{anchor, flags} {
    makeNonOwned();
}

Anchor ScrollArea::contents() const {
    return {ui(), _contentsNode};
}

Vector2 ScrollArea::scrollPercentage() const {
    return {
        _scrollXStorage == DataLayerStorageHandle::Null ? 0.0f :
            ui().dataLayer().storage<ScrollAreaStorage>(_scrollXStorage).contentsPercentage(),
        _scrollYStorage == DataLayerStorageHandle::Null ? 0.0f :
            ui().dataLayer().storage<ScrollAreaStorage>(_scrollYStorage).contentsPercentage()
    };
}

ScrollArea& ScrollArea::scrollToPercentageX(const Float percentage) {
    if(_scrollXStorage != DataLayerStorageHandle::Null)
        ui().dataLayer().storage<ScrollAreaStorage>(_scrollXStorage).scrollToPercentage(percentage);
    return *this;
}

ScrollArea& ScrollArea::scrollToPercentageY(const Float percentage) {
    if(_scrollYStorage != DataLayerStorageHandle::Null)
        ui().dataLayer().storage<ScrollAreaStorage>(_scrollYStorage).scrollToPercentage(percentage);
    return *this;
}

ScrollArea& ScrollArea::scrollToPercentage(const Vector2& percentage) {
    scrollToPercentageX(percentage.x());
    scrollToPercentageY(percentage.y());
    return *this;
}

StorageHandle ScrollArea::scrollXStorage() const {
    /* The storage is implicitly from the data layer */
    return _scrollXStorage == DataLayerStorageHandle::Null ? StorageHandle::Null :
        storageHandle(ui().dataLayer(), _scrollXStorage);
}

StorageHandle ScrollArea::scrollYStorage() const {
    /* The storage is implicitly from the data layer */
    return _scrollYStorage == DataLayerStorageHandle::Null ? StorageHandle::Null :
        storageHandle(ui().dataLayer(), _scrollYStorage);
}

}}
