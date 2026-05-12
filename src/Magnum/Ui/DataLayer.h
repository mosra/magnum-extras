#ifndef Magnum_Ui_DataLayer_h
#define Magnum_Ui_DataLayer_h
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

/** @file
 * @brief Class @ref Magnum::Ui::DataLayer, @ref Magnum::Ui::AbstractStorage, @ref Magnum::Ui::AbstractStorageQuery, @ref Magnum::Ui::StorageQuery, handle @ref Magnum::Ui::DataLayerStorageHandle, @ref Magnum::Ui::StorageHandle, enum @ref Magnum::Ui::StorageFlag, @ref Magnum::Ui::StorageOperation, @ref Magnum::Ui::StorageUpdateState, enum set @ref Magnum::Ui::StorageFlags, @ref Magnum::Ui::StorageOperations, function @ref Magnum::Ui::dataLayerStorageHandle(), @ref Magnum::Ui::dataLayerStorageHandleId(), @ref Magnum::Ui::dataLayerStorageHandleGeneration(), @ref Magnum::Ui::storageHandle(), @ref Magnum::Ui::storageHandleLayer(), @ref Magnum::Ui::storageHandleStorage(), @ref Magnum::Ui::storageHandleLayerId(), @ref Magnum::Ui::storageHandleLayerGeneration(), @ref Magnum::Ui::storageHandleId(), @ref Magnum::Ui::storageHandleGeneration()
 * @m_since_latest_{extras}
 */

/* Cannot really avoid these two due to the genericity of the interface ... */
#include <Corrade/Containers/Function.h>
#include <Corrade/Containers/StridedDimensions.h>
#include <Corrade/Utility/Assert.h>

#include "Magnum/Ui/AbstractLayer.h"

namespace Magnum { namespace Ui {

/* Unlike DataHandle, NodeHandle etc., which are global to the whole Ui
   library, StorageHandle and DataLayerStorageHandle are specific to the
   DataLayer and thus aren't defined in Handle.h but here. Similar case is with
   FontHandle in TextLayer. */

namespace Implementation {
    enum: UnsignedInt {
        DataLayerStorageHandleIdBits = 20,
        DataLayerStorageHandleGenerationBits = 12
    };
}

/**
@brief Data layer storage handle
@m_since_latest_{extras}

Uses 20 bits for storing an ID and 12 bits for a generation.
@see @ref AbstractStorage::handle(), @ref DataLayer::removeStorage(),
    @ref dataLayerStorageHandle(), @ref dataLayerStorageHandleId(),
    @ref dataLayerStorageHandleGeneration()
*/
enum class DataLayerStorageHandle: UnsignedInt {
    Null = 0    /**< Null handle */
};

/**
@debugoperatorenum{DataLayerStorageHandle}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, DataLayerStorageHandle value);

/**
@brief Compose a data layer storage handle from an ID and a generation
@m_since_latest_{extras}

Expects that the ID fits into 20 bits and the generation into 12 bits. Use
@ref dataLayerStorageHandleId() and @ref dataLayerStorageHandleGeneration() for
an inverse operation.
*/
constexpr DataLayerStorageHandle dataLayerStorageHandle(UnsignedInt id, UnsignedInt generation) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(id < (1 << Implementation::DataLayerStorageHandleIdBits) && generation < (1 << Implementation::DataLayerStorageHandleGenerationBits),
        "Ui::dataLayerStorageHandle(): expected index to fit into" << Implementation::DataLayerStorageHandleIdBits << "bits and generation into" << Implementation::DataLayerStorageHandleGenerationBits << Debug::nospace << ", got" << Debug::hex << id << "and" << Debug::hex << generation), DataLayerStorageHandle(id|(generation << Implementation::DataLayerStorageHandleIdBits)));
}

/**
@brief Extract ID from a data layer storage handle
@m_since_latest_{extras}

Expects that @p handle generation is not @cpp 0 @ce, which is the case only for
@ref DataLayerStorageHandle::Null and invalid handles. Use
@ref dataLayerStorageHandleGeneration() for extracting the generation and
@ref dataLayerStorageHandle() for an inverse operation.
*/
constexpr UnsignedInt dataLayerStorageHandleId(DataLayerStorageHandle handle) {
    /* Copy of dataLayerStorageHandleGeneration() to avoid an extra call on
       debug builds */
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(UnsignedInt(handle) >> Implementation::DataLayerStorageHandleIdBits,
        "Ui::dataLayerStorageHandleId(): invalid handle" << handle),
        UnsignedInt(handle) & ((1 << Implementation::DataLayerStorageHandleIdBits) - 1));
}

/**
@brief Extract generation from a layer data handle
@m_since_latest_{extras}

For @ref DataLayerStorageHandle::Null returns @cpp 0 @ce. A valid handle has always a
non-zero generation. Use @ref dataLayerStorageHandleId() for extracting the ID and
@ref dataLayerStorageHandle() for an inverse operation.
*/
constexpr UnsignedInt dataLayerStorageHandleGeneration(DataLayerStorageHandle handle) {
    return UnsignedInt(handle) >> Implementation::DataLayerStorageHandleIdBits;
}

/**
@brief Storage handle
@m_since_latest_{extras}

A combination of a @ref LayerHandle and a @ref DataLayerStorageHandle. Uses 8
bits for storing a layer ID, 8 bits for a layer generation, 20 bits for storing
a data ID and 12 bits for a data generation.
@see @ref AbstractStorage::handle(), @ref DataLayer::removeStorage(),
    @ref storageHandle(), @ref storageHandleLayer(),
    @ref storageHandleStorage(), @ref storageHandleLayerId(),
    @ref storageHandleLayerGeneration(), @ref storageHandleId(),
    @ref storageHandleGeneration()
*/
enum class StorageHandle: UnsignedLong {
    Null = 0    /**< Null handle */
};

/**
@debugoperatorenum{StorageHandle}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, StorageHandle value);

/**
@brief Compose a storage handle from a layer handle, a storage ID and a storage generation
@m_since_latest_{extras}

Expects that the ID fits into 20 bits and the generation into 12 bits. Use
@ref storageHandleLayer(), @ref storageHandleId() and
@ref storageHandleGeneration() for an inverse operation.
@see @ref storageHandle(LayerHandle, DataLayerStorageHandle),
    @ref storageHandleStorage(), @ref storageHandleLayerId(),
    @ref storageHandleLayerGeneration()
*/
constexpr StorageHandle storageHandle(LayerHandle layerHandle, UnsignedInt id, UnsignedInt generation) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(id < (1 << Implementation::DataLayerStorageHandleIdBits) && generation < (1 << Implementation::DataLayerStorageHandleGenerationBits),
        "Ui::storageHandle(): expected index to fit into" << Implementation::DataLayerStorageHandleIdBits << "bits and generation into" << Implementation::DataLayerStorageHandleGenerationBits << Debug::nospace << ", got" << Debug::hex << id << "and" << Debug::hex << generation), StorageHandle(id|(UnsignedLong(generation) << Implementation::DataLayerStorageHandleIdBits)|(UnsignedLong(layerHandle) << (Implementation::DataLayerStorageHandleIdBits + Implementation::DataLayerStorageHandleGenerationBits))));
}

/**
@brief Compose a storage handle from a layer handle and a data layer storage handle
@m_since_latest_{extras}

Use @ref storageHandleLayer() and @ref storageHandleStorage() for the inverse
operation.
@see @ref storageHandle(LayerHandle, UnsignedInt, UnsignedInt),
    @ref storageHandleLayerId(), @ref storageHandleLayerGeneration(),
    @ref storageHandleId(), @ref storageHandleGeneration()
*/
constexpr StorageHandle storageHandle(LayerHandle layerHandle, DataLayerStorageHandle dataLayerStorageHandle) {
    return StorageHandle((UnsignedLong(layerHandle) << (Implementation::DataLayerStorageHandleIdBits + Implementation::DataLayerStorageHandleGenerationBits))|UnsignedLong(dataLayerStorageHandle));
}

/**
@brief Extract layer handle from a storage handle
@m_since_latest_{extras}

Use @ref storageHandleStorage() for extracting the data layer storage handle
and @ref storageHandle(LayerHandle, DataLayerStorageHandle) for an inverse
operation.
*/
constexpr LayerHandle storageHandleLayer(StorageHandle handle) {
    return LayerHandle(UnsignedLong(handle) >> (Implementation::DataLayerStorageHandleIdBits + Implementation::DataLayerStorageHandleGenerationBits));
}

/**
@brief Extract data layer storage handle from a storage handle
@m_since_latest_{extras}

Use @ref storageHandleLayer() for extracting the layer handle and
@ref storageHandle(LayerHandle, DataLayerStorageHandle) for an inverse
operation.
*/
constexpr DataLayerStorageHandle storageHandleStorage(StorageHandle handle) {
    return DataLayerStorageHandle(UnsignedLong(handle));
}

/**
@brief Extract layer ID from a storage handle
@m_since_latest_{extras}

Expects that the layer portion of @p handle has a generation that is not
@cpp 0 @ce, which is the case only for @ref StorageHandle::Null and invalid
handles. Use @ref storageHandleLayerGeneration() for extracting the layer
generation and @ref layerHandle() together with @ref storageHandle() for an
inverse operation.
@see @ref storageHandleLayer()
*/
constexpr UnsignedInt storageHandleLayerId(StorageHandle handle) {
    /* Copy of storageHandleLayerGeneration() to avoid an extra call on debug
       builds */
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT((UnsignedLong(handle) >> (Implementation::DataLayerStorageHandleIdBits + Implementation::DataLayerStorageHandleGenerationBits + Implementation::LayerHandleIdBits)) & ((1 << Implementation::LayerHandleGenerationBits) - 1),
        "Ui::storageHandleLayerId(): invalid layer portion of" << handle),
        (UnsignedLong(handle) >> (Implementation::DataLayerStorageHandleIdBits + Implementation::DataLayerStorageHandleGenerationBits)) & ((1 << Implementation::LayerHandleIdBits) - 1));
}

/**
@brief Extract layer generation from a storage handle
@m_since_latest_{extras}

If the layer portion of the handle is @ref LayerHandle::Null, returns
@cpp 0 @ce. A valid handle has always a non-zero generation. Use
@ref storageHandleLayerId() for extracting the ID and @ref layerHandle()
together with @ref storageHandle() for an inverse operation.
@see @ref storageHandleLayer()
*/
constexpr UnsignedInt storageHandleLayerGeneration(StorageHandle handle) {
    return (UnsignedLong(handle) >> (Implementation::DataLayerStorageHandleIdBits + Implementation::DataLayerStorageHandleGenerationBits + Implementation::LayerHandleIdBits)) & ((1 << Implementation::LayerHandleGenerationBits) - 1);
}

/**
@brief Extract ID from a storage handle
@m_since_latest_{extras}

Expects that the storage portion of @p handle has a generation that is not
@cpp 0 @ce, which is the case only for @ref StorageHandle::Null and invalid
handles. Use @ref storageHandleGeneration() for extracting the generation and
@ref storageHandle(LayerHandle, UnsignedInt, UnsignedInt) for an inverse
operation.
@see @ref storageHandleStorage()
*/
constexpr UnsignedInt storageHandleId(StorageHandle handle) {
    /* Copy of storageHandleGeneration() to avoid an extra call on debug
       builds */
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT((UnsignedLong(handle) >> Implementation::DataLayerStorageHandleIdBits) & ((1 << Implementation::DataLayerStorageHandleGenerationBits) - 1),
        "Ui::storageHandleId(): invalid storage portion of" << handle),
        UnsignedLong(handle) & ((1 << Implementation::DataLayerStorageHandleIdBits) - 1));
}

/**
@brief Extract generation from a storage handle
@m_since_latest_{extras}

If the storage portion of @p handle is @ref DataLayerStorageHandle::Null,
returns @cpp 0 @ce. A valid handle has always a non-zero generation. Use
@ref storageHandleId() for extracting the ID and
@ref storageHandle(LayerHandle, UnsignedInt, UnsignedInt) for an inverse
operation.
@see @ref storageHandleStorage()
*/
constexpr UnsignedInt storageHandleGeneration(StorageHandle handle) {
    return (UnsignedLong(handle) >> Implementation::DataLayerStorageHandleIdBits) & ((1 << Implementation::DataLayerStorageHandleGenerationBits) - 1);
}

/**
@brief Storage flag
@m_since_latest_{extras}

@see @ref StorageFlags, @ref AbstractStorage::AbstractStorage()
*/
enum class StorageFlag: UnsignedByte {
    /**
     * Reference counted storage. By default a storage is kept alive until
     * @ref DataLayer::removeStorage() is called on it, with this flag it's
     * removed during the first @ref AbstractUserInterface::update() where its
     * @ref DataLayer::storageReferenceCount() is @cpp 0 @ce.
     */
    ReferenceCounted = 1 << 0,

    /* Last two bits used internally for distinguishing alocated and dirty
       storage */
};

/**
@debugoperatorenum{StorageFlag}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, StorageFlag value);

/**
@brief Storage flags
@m_since_latest_{extras}

@see @ref AbstractStorage::AbstractStorage()
*/
typedef Containers::EnumSet<StorageFlag> StorageFlags;

/**
@debugoperatorenum{StorageFlags}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, StorageFlags value);

CORRADE_ENUMSET_OPERATORS(StorageFlags)

/**
@brief Storage operation
@m_since_latest_{extras}

@see @ref StorageOperations, @ref StorageQuery::StorageQuery(),
    @ref StorageQuery::operations(), @ref StorageQuery::updater()
*/
enum class StorageOperation: UnsignedByte {
    /**
     * Set a value.
     * @see @ref StorageQuery::set()
     */
    Set = 1 << 0,

    /**
     * Reset a value back to default, such as a resetting a customized setting
     * back to the default.
     * @see @ref StorageQuery::reset()
     */
    Reset = 1 << 1,

    /**
     * Toggle a value, such as toggling a checkbox without needing to know what
     * the current value is.
     * @see @ref StorageQuery::toggle()
     */
    Toggle = 1 << 2,

    /**
     * Increment a value, such as in a numeric input, but for example also
     * changing to the next value in a drop-down select, scrolling a view
     * further or seeking an audio / video track forward.
     *
     * When passed to the @ref StorageQuery constructor, either both
     * @ref StorageOperation::Increment and
     * @relativeref{StorageOperation,Decrement} have to be present or neither.
     * @see @ref StorageQuery::increment()
     */
    Increment = 1 << 3,

    /**
     * Decrement a value, such as in a numeric input, but for example also
     * changing to the previous value in a drop-down select, scrolling a view
     * back or seeking an audio / video track backward.
     *
     * When passed to the @ref StorageQuery constructor, either both
     * @ref StorageOperation::Increment and
     * @relativeref{StorageOperation,Decrement} have to be present or neither.
     * @see @ref StorageQuery::decrement()
     */
    Decrement = 1 << 4,

    /**
     * Query or set a minimum possible value, but for example also scrolling to
     * begin of a view or seeking to begin of an audio / video track.
     *
     * When passed to the @ref StorageQuery constructor, either both
     * @ref StorageOperation::Min and @relativeref{StorageOperation,Max} have
     * to be present or neither.
     * @see @ref StorageQuery::min(), @ref StorageQuery::setToMin()
     */
    Min = 1 << 5,

    /**
     * Query or set a maximum possible value, but for example also scrolling to
     * end of a view or seeking to end of an audio / video track.
     *
     * When passed to the @ref StorageQuery constructor, either both
     * @ref StorageOperation::Min and @relativeref{StorageOperation,Max} have
     * to be present or neither.
     * @see @ref StorageQuery::max(), @ref StorageQuery::setToMax()
     */
    Max = 1 << 6,
};

/**
@debugoperatorenum{StorageOperation}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, StorageOperation value);

/**
@brief Storage operations
@m_since_latest_{extras}

@see @ref StorageQuery::StorageQuery(), @ref StorageQuery::operations(),
    @ref StorageQuery::updater()
*/
typedef Containers::EnumSet<StorageOperation> StorageOperations;

/**
@debugoperatorenum{StorageOperations}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, StorageOperations value);

CORRADE_ENUMSET_OPERATORS(StorageOperations)

/**
@brief Storage update state
@m_since_latest_{extras}

@see @ref StorageQuery::updater(), @ref StorageQuery::set()
*/
enum class StorageUpdateState: UnsignedByte {
    /**
     * The update succeeded, setting the desired value.
     *
     * This value can be returned by the updater for any @ref StorageOperation.
     */
    Success,

    /**
     * The update succeeded but the value was approximated, for example if a
     * numeric storage is quantized. This is commonly considered to not be an
     * issue and shouldn't result in a warning feedback to the user.
     *
     * This value can be returned by the updater *only* for
     * @ref StorageOperation::Set, all other operations are expected to return
     * @ref StorageUpdateState::Success.
     */
    Approximated,

    /**
     * The update succeeded but the value was clamped, for example if
     * attempting to set a value larger than what can fit, or if a number is
     * outside of allowed range. Unlike @ref StorageUpdateState::Approximated
     * this state should result in a visual feedback to the user to warn about
     * potential data loss.
     *
     * The concrete clamping behavior is specific to a particular storage and
     * query implementation, for numbers in particular it can be either a
     * min/max operation or a wraparound.
     *
     * This value can only be returned by the updater *only* for
     * @ref StorageOperation::Set, all other operations are expected to return
     * @ref StorageUpdateState::Success.
     */
    Clamped,

    /**
     * The update failed because given value isn't allowed. Concrete
     * interpretation of the failure is specific to a particular storage and
     * query implementation.
     *
     * This value can only be returned by the updater *only* for
     * @ref StorageOperation::Set, all other operations are expected to return
     * @ref StorageUpdateState::Success.
     */
    Failed
};

/**
@debugoperatorenum{StorageUpdateState}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, StorageUpdateState value);

namespace Implementation {
    enum: std::size_t {
        /* Used by AbstractStorage interfaces as well as DataLayer internals */
        #ifndef CORRADE_TARGET_32BIT
        DataLayerStorageMaxInPlaceSize = 4*sizeof(std::size_t)
        #else
        DataLayerStorageMaxInPlaceSize = 5*sizeof(std::size_t)
        #endif
    };
    template<UnsignedInt, class, class, class, class...> struct StorageLambda;
    template<UnsignedInt dimensions, class F, class G> struct StorageArgs {};
    enum class StorageCallOoverload {
        ByReference, /* void(const T&) */
        ByValue, /* void(T) */
        ByValueMinMax, /* void(T, T, T) */
    };
}

class AbstractStorage;

/**
@brief Data binding layer
@m_since_latest_{extras}

@section Ui-DataLayer-setup Setting up a data layer instance

If you create a @ref UserInterfaceGL with a theme and don't exclude
@ref ThemeFeature::DataLayer, an implicit instance is already provided and
available through @ref UserInterface::dataLayer(). Otherwise, the layer doesn't
have any shared state or configuration, so it's just about constructing it from
a fresh @ref AbstractUserInterface::createLayer() handle and passing it to
@ref UserInterface::setDataLayerInstance():

@snippet Ui.cpp DataLayer-setup-implicit

In comparison, if you want to set up a custom data layer that's independent of
the one exposed through @ref UserInterface::dataLayer(), pass the newly created
instance to @ref AbstractUserInterface::setLayerInstance() instead:

@snippet Ui.cpp DataLayer-setup

Afterwards, with either of the above, assuming
@ref AbstractUserInterface::draw() is called in an appropriate place, the
layer is ready to use.
*/
class MAGNUM_UI_EXPORT DataLayer: public AbstractLayer {
    public:
        /**
         * @brief Constructor
         * @param handle        Layer handle returned from
         *      @ref AbstractUserInterface::createLayer()
         */
        explicit DataLayer(LayerHandle handle);

        /** @brief Copying is not allowed */
        DataLayer(const DataLayer&) = delete;

        /** @copydoc AbstractLayer::AbstractLayer(AbstractLayer&&) */
        DataLayer(DataLayer&&) noexcept;

        ~DataLayer();

        /** @brief Copying is not allowed */
        DataLayer& operator=(const DataLayer&) = delete;

        /** @brief Move assignment */
        DataLayer& operator=(DataLayer&&) noexcept;

        /** @{
         * @name Storage management
         */

        /**
         * @brief Current capacity of the storage storage
         *
         * Can be at most 1048576. If an @ref AbstractStorage subclass instance
         * is constructed and there's no free slots left, the internal storage
         * gets grown.
         * @see @ref storageUsedCount()
         */
        std::size_t storageCapacity() const;

        /**
         * @brief Count of used items in the storage storage
         *
         * Always at most @ref storageCapacity(). Expired handles are counted
         * among used as well. The operation is done with a
         * @f$ \mathcal{O}(n) @f$ complexity where @f$ n @f$ is
         * @ref storageCapacity().
         */
        std::size_t storageUsedCount() const;

        /**
         * @brief Count of used allocated items in the storage storage
         *
         * Always at most @ref storageUsedCount(). Counts all storages that
         * have non-trivially-copyable state or state that's too large to be
         * stored in-place. The operation is done with a @f$ \mathcal{O}(n) @f$
         * complexity where @f$ n @f$ is @ref storageCapacity().
         * @see @ref isStorageAllocated(), @ref usedAllocatedCount()
         */
        std::size_t storageUsedAllocatedCount() const;

        /**
         * @brief Whether a storage handle is valid
         *
         * A handle is valid if it has been created via an @ref AbstractStorage
         * subclass constructor and @ref removeStorage() wasn't called on it
         * yet. For @ref DataLayerStorageHandle::Null always returns
         * @cpp false @ce.
         */
        bool isHandleValid(DataLayerStorageHandle handle) const;

        /**
         * @brief Whether a storage handle is valid
         *
         * A shorthand for extracting a @ref LayerHandle from @p handle using
         * @ref storageHandleLayer(), comparing it to @ref handle() and if it's
         * the same, calling @ref isHandleValid(DataLayerStorageHandle) const
         * with a @ref DataLayerStorageHandle extracted from @p handle using
         * @ref storageHandleStorage(). See these functions for more
         * information. For @ref StorageHandle::Null, @ref LayerHandle::Null or
         * @ref DataLayerStorageHandle::Null always returns @cpp false @ce.
         */
        bool isHandleValid(StorageHandle handle) const;

        /* So the DataHandle / LayerDataHandle overloads are accessible as
           well */
        using AbstractLayer::isHandleValid;

        /**
         * @brief Remove a storage
         *
         * Expects that @p handle is valid and the storage isn't referenced by
         * any data. Calls a deleter on the storage if it's allocated.
         * @see @ref isHandleValid(StorageHandle) const,
         *      @ref storageReferenceCount(), @ref isStorageAllocated()
         */
        void removeStorage(StorageHandle handle);

        /**
         * @brief Remove a storage assuming it belongs to this layer
         *
         * Like @ref removeStorage(StorageHandle) but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         * @see @ref isHandleValid(DataLayerStorageHandle) const,
         *      @ref storageHandleStorage()
         */
        void removeStorage(DataLayerStorageHandle handle);

        /**
         * @brief Whether given storage is allocated
         *
         * Returns @cpp true @ce if given storage has a
         * non-trivially-copyable type or its size is too large to be stored
         * in-place, @cpp false @ce otherwise. Expects that @p handle is valid.
         * @see @ref isHandleValid(StorageHandle) const,
         *      @ref storageUsedAllocatedCount(), @ref isAllocated(),
         *      @ref AbstractStorage::isAllocated()
         */
        bool isStorageAllocated(StorageHandle handle) const;

        /**
         * @brief Whether given storage is allocated assuming it belongs to this layer
         *
         * Like @ref isStorageAllocated(StorageHandle) const but without
         * checking that @p handle indeed belongs to this layer. See its
         * documentation for more information.
         * @see @ref isHandleValid(DataLayerStorageHandle) const,
         *      @ref storageHandleStorage()
         */
        bool isStorageAllocated(DataLayerStorageHandle handle) const;

        /**
         * @brief Whether given storage is marked as dirty
         *
         * Returns @cpp true @ce if data update functions for given storage
         * will be called in the next @ref AbstractUserInterface::update() or
         * @relativeref{AbstractUserInterface,draw()}, @cpp false @ce
         * otherwise. Expects that @p handle is valid.
         *
         * The storage is not marked as dirty upon creation, the data
         * themselves however *are* marked as dirty when created in order to
         * have their update function called at least once.
         * @see @ref isHandleValid(StorageHandle) const,
         *      @ref setStorageDirty(), @ref isDirty(DataHandle) const,
         *      @ref AbstractStorage::isDirty()
         */
        bool isStorageDirty(StorageHandle handle) const;

        /**
         * @brief Whether given storage is marked as dirty assuming it belongs to this layer
         *
         * Like @ref isStorageDirty(StorageHandle) const but without checking
         * that @p handle indeed belongs to this layer. See its documentation
         * for more information.
         * @see @ref isHandleValid(DataLayerStorageHandle) const,
         *      @ref storageHandleStorage()
         */
        bool isStorageDirty(DataLayerStorageHandle handle) const;

        /**
         * @brief Mark given storage as dirty
         *
         * Calling this function causes update functions for data associated
         * with this storage to be called in the next
         * @ref AbstractUserInterface::update() or
         * @relativeref{AbstractUserInterface,draw()}. Expects that @p handle
         * is valid. See also @ref setDirty() which can be used to force an
         * update of just a single data.
         *
         * Calling this function causes @ref LayerState::NeedsCommonDataUpdate
         * to be set, unless the storage has a zero reference count and thus no
         * update functions need to be called.
         * @see @ref isHandleValid(StorageHandle) const,
         *      @ref storageReferenceCount(), @ref AbstractStorage::setDirty()
         */
        void setStorageDirty(StorageHandle handle);

        /**
         * @brief Mark given storage as dirty assuming it belongs to this layer
         *
         * Like @ref setStorageDirty(StorageHandle) but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         * @see @ref isHandleValid(DataLayerStorageHandle) const,
         *      @ref storageHandleStorage()
         */
        void setStorageDirty(DataLayerStorageHandle handle);

        /**
         * @brief Storage flags
         *
         * Expects that @p handle is valid.
         * @see @ref isHandleValid(StorageHandle) const
         */
        StorageFlags storageFlags(StorageHandle handle) const;

        /**
         * @brief Storage flags assuming it belongs to this layer
         *
         * Like @ref storageFlags(StorageHandle) const but without checking
         * that @p handle indeed belongs to this layer. See its documentation
         * for more information.
         * @see @ref isHandleValid(DataLayerStorageHandle) const,
         *      @ref storageHandleStorage()
         */
        StorageFlags storageFlags(DataLayerStorageHandle handle) const;

        /**
         * @brief Storage size
         *
         * Expects that @p handle is valid. If the storage was created without
         * specifying a size, with a 1D size or with a 2D size, returns
         * @cpp 1 @ce in remaining leading dimensions. For example, if the
         * storage was created with a 2D size of @cpp {4, 12} @ce, the returned
         * 3D size is going to be @cpp {1, 4, 12} @ce.
         * @see @ref isHandleValid(StorageHandle) const,
         *      @ref AbstractStorage::size()
         */
        Containers::Size3D storageSize(StorageHandle handle) const;

        /**
         * @brief Storage size assuming it belongs to this layer
         *
         * Like @ref storageSize(StorageHandle) const but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         * @see @ref isHandleValid(DataLayerStorageHandle) const,
         *      @ref storageHandleStorage()
         */
        Containers::Size3D storageSize(DataLayerStorageHandle handle) const;

        /**
         * @brief Storage reference count
         *
         * Expects that @p handle is valid. Initially @cpp 0 @ce, is
         * incremented after every @ref onUpdate() call referencing given
         * @p handle and decremented after every @ref remove() of data
         * referencing given @p handle, or when a node the data is attached to
         * is removed along with all attachments. Can be at most
         * @ref usedCount(). It's only allowed to @ref removeStorage() with a
         * zero reference count.
         * @see @ref isHandleValid(StorageHandle) const,
         *      @ref AbstractStorage::referenceCount()
         */
        std::size_t storageReferenceCount(StorageHandle handle) const;

        /**
         * @brief Storage reference count assuming it belongs to this layer
         *
         * Like @ref storageReferenceCount(StorageHandle) const but without
         * checking that @p handle indeed belongs to this layer. See its
         * documentation for more information.
         * @see @ref isHandleValid(DataLayerStorageHandle) const,
         *      @ref storageHandleStorage()
         */
        std::size_t storageReferenceCount(DataLayerStorageHandle handle) const;

        /**
         * @brief Storage access
         *
         * Expects that @p handle is valid, it's user responsibility to ensure
         * that @p Storage is matching the @ref AbstractStorage subclass used
         * for creating the storage with given @p handle.
         *
         * Note that because the returned instance generally allows
         * unconditionally mutating the storage contents, there's no
         * @cpp const @ce overload of this function.
         */
        template<class Storage> Storage storage(StorageHandle handle);

        /**
         * @brief Storage access assuming it belongs to this layer
         *
         * Like @ref storage(StorageHandle) but without checking that @p handle
         * indeed belongs to this layer. See its documentation for more
         * information.
         * @see @ref isHandleValid(DataLayerStorageHandle) const,
         *      @ref storageHandleStorage()
         */
        template<class Storage> Storage storage(DataLayerStorageHandle handle);

        /**
         * @brief Generation counters for all storages
         *
         * Meant to be used by code that only gets storage IDs or masks but
         * needs the full handle, or for various diagnostic purposes such as
         * tracking handle recycling. Size of the returned view is the same as
         * @ref storageCapacity(), individual items correspond to generations
         * of particular storage IDs. All values fit into the
         * @ref StorageHandle / @ref DataLayerStorageHandle generation bits,
         * @cpp 0 @ce denotes an expired generation counter.
         *
         * Passing an ID along with the corresponding generation to
         * @ref dataLayerStorageHandle() produces a
         * @ref DataLayerStorageHandle, passing that along with @ref handle()
         * to @ref storageHandle() produces a @ref StorageHandle. Use
         * @ref isHandleValid(DataLayerStorageHandle) const /
         * @ref isHandleValid(StorageHandle) const to determine whether given
         * slot is actually used.
         */
        Containers::StridedArrayView1D<const UnsignedShort> storageGenerations() const;

        /**
         * @}
         */

        /**
         * @brief Count of allocated data update functions
         *
         * Always at most @ref usedCount(). Counts all data update functions
         * that capture non-trivially-copyable state or state that's too large
         * to be stored in-place. The operation is done with a
         * @f$ \mathcal{O}(n) @f$ complexity where @f$ n @f$ is
         * @ref capacity().
         * @todoc fix the isAllocated link once Doxygen stops being shit -- it
         *      works only from Containers themselves
         * @see @ref Corrade::Containers::Function "Containers::Function<R(Args...)>::isAllocated()",
         *      @ref isAllocated(), @ref storageUsedAllocatedCount()
         */
        std::size_t usedAllocatedCount() const;

        /**
         * @brief Bind a function to storage data update
         * @param query         Storage query
         * @param function      Function to call when data get updated
         * @param node          Node to attach to
         * @return New data handle
         *
         * Expects that @p query references a valid storage from this layer and
         * @p function is not @cpp nullptr @ce. The @p function gets called
         * with the result of @p query with @ref AbstractUserInterface::update()
         * or @relativeref{AbstractUserInterface,draw()} whenever either the
         * data itself or the @p storage is marked as dirty. The data is marked
         * as dirty initially.
         *
         * Delegates to @ref AbstractLayer::create(), see its documentation for
         * detailed description of all constraints. Calling this function
         * causes @ref LayerState::NeedsCommonDataUpdate to be set.
         * @see @ref isStorageDirty(), @ref storageSize()
         */
        template<class T> DataHandle onUpdate(const StorageQuery<T>& query,
            #ifdef DOXYGEN_GENERATING_OUTPUT
            Containers::Function<void(const T& value)>&&
            #else /* Without this, deduction of T won't work */
            typename std::common_type<Containers::Function<void(const T&)>>::type&&
            #endif
        function, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return onUpdateInternal(*query._layer, query._storage, query._index, query._call(Implementation::StorageCallOoverload::ByReference), Utility::move(function), node);
        }
        /** @overload */
        template<class T> DataHandle onUpdate(const StorageQuery<T>& query,
            #ifdef DOXYGEN_GENERATING_OUTPUT
            Containers::Function<void(T value)>&&
            #else /* Without this, deduction of T won't work */
            typename std::common_type<Containers::Function<void(T)>>::type&&
            #endif
        function, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return onUpdateInternal(*query._layer, query._storage, query._index, query._call(Implementation::StorageCallOoverload::ByValue), Utility::move(function), node);
        }

        /**
         * @brief Bind a function to storage data update along with min/max values
         * @param query         Storage query
         * @param function      Function to call when data get updated
         * @param node          Node to attach to
         * @return New data handle
         *
         * Expects that @p query references a valid storage from this layer,
         * @ref StorageQuery::operations() includes @ref StorageOperation::Min
         * and @relativeref{StorageOperation,Max} and @p function is not
         * @cpp nullptr @ce. The @p function gets called with the result of
         * @p query along with min and max values with
         * @ref AbstractUserInterface::update() or
         * @relativeref{AbstractUserInterface,draw()} whenever either the data
         * itself or the @p storage is marked as dirty. The data is marked as
         * dirty initially.
         *
         * Delegates to @ref AbstractLayer::create(), see its documentation for
         * detailed description of all constraints. Calling this function
         * causes @ref LayerState::NeedsCommonDataUpdate to be set.
         * @see @ref isStorageDirty(), @ref storageSize()
         */
        template<class T> DataHandle onUpdate(const StorageQuery<T>& query,
            #ifdef DOXYGEN_GENERATING_OUTPUT
            Containers::Function<void(T value, T min, T max)>&&
            #else /* Without this, deduction of T won't work */
            typename std::common_type<Containers::Function<void(T, T, T)>>::type&&
            #endif
        function, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            CORRADE_ASSERT(query.operations() >= (StorageOperation::Min|StorageOperation::Max),
                "Ui::DataLayer::onUpdate(): query doesn't support" << (StorageOperation::Min|StorageOperation::Max) << "for this overload", {});
            return onUpdateInternal(*query._layer, query._storage, query._index, query._call(Implementation::StorageCallOoverload::ByValueMinMax), Utility::move(function), node);
        }

        /**
         * @brief Bind a function to storage's implicit data update
         *
         * Assuming the @p storage has a `Type` @cpp typedef @ce which denotes
         * what @ref StorageQuery type the storage is implicitly convertible
         * to, delegates to @ref onUpdate(const StorageQuery<T>&, Containers::Function<void(const T& value)>&&, NodeHandle)
         * and overloads with given type. See their documentation for more
         * information.
         */
        template<class Storage
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , typename std::enable_if<std::is_base_of<AbstractStorage, Storage>::value, int>::type = 0
            #endif
        > DataHandle onUpdate(const Storage& storage,
            #ifdef DOXYGEN_GENERATING_OUTPUT
            Containers::Function<void(const typename Storage::Type& value)>&&
            #else /* Without this, deduction of T won't work */
            typename std::common_type<Containers::Function<void(const typename Storage::Type&)>>::type&&
            #endif
        function, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return onUpdate<typename Storage::Type>(storage, Utility::move(function), node);
        }
        /** @overload */
        template<class Storage
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , typename std::enable_if<std::is_base_of<AbstractStorage, Storage>::value, int>::type = 0
            #endif
        > DataHandle onUpdate(const Storage& storage,
            #ifdef DOXYGEN_GENERATING_OUTPUT
            Containers::Function<void(typename Storage::Type value)>&&
            #else /* Without this, deduction of T won't work */
            typename std::common_type<Containers::Function<void(typename Storage::Type)>>::type&&
            #endif
        function, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return onUpdate<typename Storage::Type>(storage, Utility::move(function), node);
        }
        /** @overload */
        template<class Storage
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , typename std::enable_if<std::is_base_of<AbstractStorage, Storage>::value, int>::type = 0
            #endif
        > DataHandle onUpdate(const Storage& storage,
            #ifdef DOXYGEN_GENERATING_OUTPUT
            Containers::Function<void(typename Storage::Type value, typename Storage::Type min, typename Storage::Type max)>&&
            #else /* Without this, deduction of T won't work */
            typename std::common_type<Containers::Function<void(typename Storage::Type, typename Storage::Type, typename Storage::Type)>>::type&&
            #endif
        function, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return onUpdate<typename Storage::Type>(storage, Utility::move(function), node);
        }

        /**
         * @brief Remove a data
         *
         * Expects that @p handle is valid. Calls a destructor on the captured
         * update function state, if it's not trivially destructible, and
         * decrements reference count on the storage the data was associated
         * with. Delegates to @ref AbstractLayer::remove(DataHandle), see its
         * documentation for detailed description of all constraints.
         *
         * If the storage the @p handle was associated with is
         * @ref StorageFlag::ReferenceCounted and its reference count becomes
         * @cpp 0 @ce by calling this function, causes
         * @ref LayerState::NeedsCommonDataUpdate to be set on the layer in
         * addition to what @ref AbstractLayer::remove() may have set.
         * @see @ref isHandleValid(DataHandle) const,
         *      @ref usedAllocatedCount(), @ref isAllocated()
         */
        void remove(DataHandle handle);

        /**
         * @brief Remove an animation assuming it belongs to this animator
         *
         * Compared to @ref remove(DataHandle) delegates to
         * @ref AbstractLayer::remove(LayerDataHandle) instead.
         * @see @ref isHandleValid(LayerDataHandle) const,
         *      @ref dataHandleData()
         */
        void remove(LayerDataHandle handle);

        /**
         * @brief Whether given data update function is allocated
         *
         * Returns @cpp true @ce if the update function in given data captures
         * non-trivially-copyable state or state that's too large to be stored
         * in-place, @cpp false @ce otherwise. Expects that @p handle is valid.
         * @see @ref isHandleValid(DataHandle) const,
         *      @ref Corrade::Containers::Function "Containers::Function<R(Args...)>::isAllocated()",
         *      @ref usedAllocatedCount()
         */
        bool isAllocated(DataHandle handle) const;

        /**
         * @brief Whether given data update function is allocated assuming it belongs to this layer
         *
         * Like @ref isAllocated(DataHandle) const but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         * @see @ref isHandleValid(LayerDataHandle) const,
         *      @ref dataHandleData()
         */
        bool isAllocated(LayerDataHandle handle) const;

        /**
         * @brief Whether given data is marked as dirty
         *
         * Returns @cpp true @ce if the data update function will be called in
         * the next @ref AbstractUserInterface::update() or
         * @relativeref{AbstractUserInterface,draw()}, @cpp false @ce
         * otherwise. Expects that @p handle is valid.
         *
         * All data are marked as dirty upon creation in order to have the
         * update function called at least once, and the dirty bits is reset in
         * the next @ref AbstractUserInterface::update() or
         * @relativeref{AbstractUserInterface,draw()}. Afterwards, they're
         * themselves marked as dirty only if their properties change such as
         * when @ref setIndex() is called, and there's no way to mark them as
         * dirty explicitly. All other cases where the update function is
         * called are driven by dirty state of the associated storage.
         * @see @ref isHandleValid(DataHandle) const,
         *      @ref isStorageDirty(StorageHandle) const
         */
        bool isDirty(DataHandle handle) const;

        /**
         * @brief Whether given data is marked as dirty assuming it belongs to this layer
         *
         * Like @ref isDirty(DataHandle) const but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         * @see @ref isHandleValid(LayerDataHandle) const,
         *      @ref dataHandleData()
         */
        bool isDirty(LayerDataHandle handle) const;

        /**
         * @brief Mark given data as dirty
         *
         * Calling this function causes the update function for given data to
         * be called in the next @ref AbstractUserInterface::update() or
         * @relativeref{AbstractUserInterface,draw()}. Expects that @p handle
         * is valid. Compared to @ref setStorageDirty(), which affects all data
         * associated with given storage, marking just a single data dirty is
         * useful for example when previous updates were skipped for some
         * reason and the storage itself didn't get dirty since.
         *
         * Calling this function causes @ref LayerState::NeedsCommonDataUpdate
         * to be set.
         * @see @ref isHandleValid(StorageHandle) const
         */
        void setDirty(DataHandle handle);

        /**
         * @brief Mark given data as dirty assuming it belongs to this layer
         *
         * Like @ref setDirty(DataHandle) but without checking that @p handle
         * indeed belongs to this layer. See its documentation for more
         * information.
         * @see @ref isHandleValid(LayerDataHandle) const,
         *      @ref dataHandleData()
         */
        void setDirty(LayerDataHandle handle);

        /**
         * @brief Storage given data is associated with
         *
         * Expects that @p handle is valid. The returned storage handle is
         * always valid and belongs to this layer.
         * @see @ref isHandleValid(DataHandle) const,
         *      @ref storage(StorageHandle)
         */
        StorageHandle storage(DataHandle handle) const;

        /**
         * @brief Storage given data is associated with assuming it belongs to this layer
         *
         * Like @ref storage(DataHandle) const but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         * @see @ref isHandleValid(LayerDataHandle) const,
         *      @ref dataHandleData()
         */
        StorageHandle storage(LayerDataHandle handle) const;

        /**
         * @brief Index of data in associated storage
         *
         * Expects that @p handle is valid. If the data was created without
         * specifying an index, with a 1D index or with a 2D index, returns
         * @cpp 0 @ce in remaining leading dimensions. For example, if the
         * data was created with a 2D index of @cpp {3, 7} @ce, the returned
         * 3D index is going to be @cpp {0, 3, 7} @ce.
         * @see @ref isHandleValid(DataHandle) const,
         *      @ref storage(DataHandle) const
         */
        Containers::Size3D index(DataHandle handle) const;

        /**
         * @brief Index of data in associated storage assuming it belongs to this layer
         *
         * Like @ref index(DataHandle) const but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         * @see @ref isHandleValid(LayerDataHandle) const,
         *      @ref dataHandleData()
         */
        Containers::Size3D index(LayerDataHandle handle) const;

        /**
         * @brief Set data index in associated 1D storage
         *
         * Expects that @p handle is valid, size of associated storage is
         * @cpp {1, 1, size} @ce and @p index is less than `size`. Use
         * @ref setIndex(DataHandle, const Containers::Size2D&) for setting a
         * data index in a 2D storage and @ref setIndex(DataHandle, const Containers::Size3D&)
         * for setting an index in a 3D storage.
         *
         * If the @p index is different from the previously used index, calling
         * this function causes the data to be marked as dirty and
         * @ref LayerState::NeedsCommonDataUpdate to be set.
         * @see @ref isHandleValid(DataHandle) const,
         *      @ref storage(DataHandle) const, @ref storageSize(),
         *      @ref isDirty(DataHandle) const
         */
        void setIndex(DataHandle handle, std::size_t index);

        /**
         * @brief Set data index in associated 1D storage assuming it belongs to this layer
         *
         * Like @ref setIndex(DataHandle, std::size_t) but without checking
         * that @p handle indeed belongs to this layer. See its documentation
         * for more information.
         * @see @ref isHandleValid(LayerDataHandle) const,
         *      @ref dataHandleData()
         */
        void setIndex(LayerDataHandle handle, std::size_t index);

        /**
         * @brief Set data index in associated 2D storage
         *
         * Expects that @p handle is valid, size of associated storage is
         * @cpp {1, size[0], size[1]} @ce and @p index is less than `size`. Use
         * @ref setIndex(DataHandle, const Containers::Size3D&) for setting a
         * data index in a 3D storage.
         *
         * If the @p index is different from the previously used index, calling
         * this function causes the data to be marked as dirty and
         * @ref LayerState::NeedsCommonDataUpdate to be set.
         * @see @ref isHandleValid(DataHandle) const,
         *      @ref storage(DataHandle) const, @ref storageSize(),
         *      @ref isDirty(DataHandle) const
         */
        void setIndex(DataHandle handle, const Containers::Size2D& index);

        /**
         * @brief Set data index in associated 2D storage assuming it belongs to this layer
         *
         * Like @ref setIndex(DataHandle, const Containers::Size2D&) but
         * without checking that @p handle indeed belongs to this layer. See
         * its documentation for more information.
         * @see @ref isHandleValid(LayerDataHandle) const,
         *      @ref dataHandleData()
         */
        void setIndex(LayerDataHandle handle, const Containers::Size2D& index);

        /**
         * @brief Set data index in associated 3D storage
         *
         * Expects that @p handle is valid and @p index is less than size of
         * the associated storage.
         *
         * If the @p index is different from the previously used index, calling
         * this function causes the data to be marked as dirty and
         * @ref LayerState::NeedsCommonDataUpdate to be set.
         * @see @ref isHandleValid(DataHandle) const,
         *      @ref storage(DataHandle) const, @ref storageSize(),
         *      @ref isDirty(DataHandle) const
         */
        void setIndex(DataHandle handle, const Containers::Size3D& index);

        /**
         * @brief Set data index in associated 3D storage assuming it belongs to this layer
         *
         * Like @ref setIndex(DataHandle, const Containers::Size3D&) but
         * without checking that @p handle indeed belongs to this layer. See
         * its documentation for more information.
         * @see @ref isHandleValid(LayerDataHandle) const,
         *      @ref dataHandleData()
         */
        void setIndex(LayerDataHandle handle, const Containers::Size3D& index);

    private:
        friend AbstractStorage;

        /* These four are called from within AbstractStorage */
        DataLayerStorageHandle createStorage(const Containers::Size3D& size, StorageFlags flags);
        auto createStorageInPlace(DataLayerStorageHandle handle) -> char(&)[Implementation::DataLayerStorageMaxInPlaceSize];
        void* createStorageAllocated(DataLayerStorageHandle handle, void* data, std::size_t dataSize, void(*deleter)(void*, std::size_t));
        void* storageData(DataLayerStorageHandle handle);

        MAGNUM_UI_LOCAL void removeStorageInternal(UnsignedInt id);
        MAGNUM_UI_LOCAL void setStorageDirtyInternal(UnsignedInt id);
        AbstractStorage storageInternal(StorageHandle handle);
        AbstractStorage storageInternal(DataLayerStorageHandle handle);

        /* The layer argument is just for the assert, but since it has to be an
           exported symbol to be used from the templated onUpdate() functions,
           it cannot be wrapped in #ifdef CORRADE_NO_ASSERT because it'd cause
           linker errors if the library is built with assertions but the user
           project not and vice versa. */
        DataHandle onUpdateInternal(const DataLayer& layer, DataLayerStorageHandle storage, const Containers::Size3D& index, void(*call)(DataLayer&, DataLayerStorageHandle, const Containers::Size3D&, Containers::FunctionData&), Containers::FunctionData&& function, NodeHandle node);
        MAGNUM_UI_LOCAL void removeInternal(UnsignedInt id);
        MAGNUM_UI_LOCAL void setDirtyInternal(UnsignedInt id);
        MAGNUM_UI_LOCAL StorageHandle storageInternal(UnsignedInt id) const;
        MAGNUM_UI_LOCAL Containers::Size3D indexInternal(UnsignedInt id) const;
        MAGNUM_UI_LOCAL void setIndexInternal(UnsignedInt id, std::size_t index);
        MAGNUM_UI_LOCAL void setIndexInternal(UnsignedInt id, const Containers::Size2D& index);
        MAGNUM_UI_LOCAL void setIndexInternal(UnsignedInt id, const Containers::Size3D& index);

        MAGNUM_UI_LOCAL LayerFeatures doFeatures() const override;
        MAGNUM_UI_LOCAL void doClean(Containers::BitArrayView dataIdsToRemove) override;
        MAGNUM_UI_LOCAL void doPreUpdate(LayerStates state) override;

        struct State;
        Containers::Pointer<State> _state;
};

/**
@brief Base for @ref DataLayer storage implementations
@m_since_latest_{extras}

Types derived from this class are used as non-owning proxies that allocate
arbitrarily typed storage memory in @ref DataLayer and maintain access to it.

As this class is just a proxy with the storage being elsewhere, subclasses are
expected to be trivially copyable without any additional member variables ---
all state is meant to be placed in the storage memory itself.
*/
class MAGNUM_UI_EXPORT AbstractStorage {
    public:
        enum: std::size_t {
            /**
             * Maximal size that can be stored in-place. Storages larger
             * than this size have to be allocated. See @ref createInPlace()
             * and @ref createAllocated() for more information.
             */
            MaxInPlaceSize = Implementation::DataLayerStorageMaxInPlaceSize
        };

        /** @brief Data layer reference */
        DataLayer& layer() const { return *_layer; }

        /**
         * @brief Storage handle
         *
         * Guaranteed to be associated with @ref layer() and never
         * @ref StorageHandle::Null.
         */
        StorageHandle handle() const {
            return storageHandle(_layer->handle(), _handle);
        }

        /**
         * @brief Storage handle
         *
         * Same as @ref handle().
         */
        /*implicit*/ operator StorageHandle() const { return handle(); }

        /**
         * @brief Whether the storage is allocated
         *
         * Same as calling @ref DataLayer::isStorageAllocated() with
         * @ref handle(), see its documentation for more information.
         */
        bool isAllocated() const { return _layer->isStorageAllocated(_handle); }

        /**
         * @brief Whether the storage is dirty
         *
         * Same as calling @ref DataLayer::isStorageDirty() with @ref handle(),
         * see its documentation for more information.
         */
        bool isDirty() const { return _layer->isStorageDirty(_handle); }

        /**
         * @brief Mark the storage as dirty
         *
         * Same as calling @ref DataLayer::setStorageDirty() with
         * @ref handle(), see its documentation for more information.
         */
        void setDirty() const { _layer->setStorageDirty(_handle); }

        /**
         * @brief Storage flags
         *
         * Same as calling @ref DataLayer::storageFlags() with @ref handle(),
         * see its documentation for more information.
         */
        StorageFlags flags() const { return _layer->storageFlags(_handle); }

        /**
         * @brief Storage size
         *
         * Same as calling @ref DataLayer::storageSize() with @ref handle(),
         * see its documentation for more information.
         */
        Containers::Size3D size() const { return _layer->storageSize(_handle); }

        /**
         * @brief Storage reference count
         *
         * Same as calling @ref DataLayer::storageReferenceCount() with
         * @ref handle(), see its documentation for more information.
         */
        std::size_t referenceCount() const {
            return _layer->storageReferenceCount(_handle);
        }

    protected:
        /**
         * @brief Create a single-item storage
         * @param layer     Data layer reference
         * @param flags     Storage flags
         *
         * Equivalent to calling @ref AbstractStorage(DataLayer&, const Containers::Size3D&, StorageFlags)
         * with @p size being @cpp {1, 1, 1} @ce. See its documentation for
         * a detailed description of all constraints.
         */
        explicit AbstractStorage(DataLayer& layer, StorageFlags flags = {}): AbstractStorage{layer, {1, 1, 1}, flags} {}

        /**
         * @brief Create a 1D storage
         * @param layer     Data layer reference
         * @param size      Storage size
         * @param flags     Storage flags
         *
         * Equivalent to calling @ref AbstractStorage(DataLayer&, const Containers::Size3D&, StorageFlags)
         * with @p size being @cpp {1, 1, size} @ce. See its documentation for
         * a detailed description of all constraints.
         */
        explicit AbstractStorage(DataLayer& layer, std::size_t size, StorageFlags flags = {}): AbstractStorage{layer, {1, 1, size}, flags} {}

        /**
         * @brief Create a 2D storage
         * @param layer     Data layer reference
         * @param size      Storage size
         * @param flags     Storage flags
         *
         * Equivalent to calling @ref AbstractStorage(DataLayer&, const Containers::Size3D&, StorageFlags)
         * with @p size being @cpp {1, size[0], size[1]} @ce. See its
         * documentation for a detailed description of all constraints.
         */
        explicit AbstractStorage(DataLayer& layer, const Containers::Size2D& size, StorageFlags flags = {}): AbstractStorage{layer, {1, size[0], size[1]}, flags} {}

        /**
         * @brief Create a 3D storage
         * @param layer     Data layer reference
         * @param size      Storage size
         * @param flags     Storage flags
         *
         * Allocates a new @ref StorageHandle in a free slot in the internal
         * @ref DataLayer storage storage or grows the storage if there's no
         * free slots left. Expects that there's at most 1048576 storages. The
         * @ref AbstractStorage instance doesn't own the storage on and thus
         * doesn't remove it on destruction, instead either the @ref handle()
         * can be removed again with @ref DataLayer::remove() or can be marked
         * as @ref StorageFlag::ReferenceCounted, which causes it to be removed
         * at the next @ref AbstractUserInterface::update() or
         * @relativeref{AbstractUserInterface,draw()} if its
         * @ref referenceCount() is zero.
         *
         * The @p size is expected to not be @cpp 0 @ce in any dimension and
         * its product to be less or equal to @cpp 1ull << 43 @ce. The subclass
         * is meant to subsequently call @ref createInPlace() or
         * @ref createAllocated() to actually initialize the storage. Calling
         * neither is equivalent to calling @ref createInPlace() and not
         * initializing the memory it returns in any way. If
         * @ref StorageFlag::ReferenceCounted is present in @p flags, the
         * storage has to get used by at least one data before the next call to
         * @ref AbstractUserInterface::update() to prevent it from being
         * removed again.
         *
         * Upon creation, @ref isDirty() const returns @cpp false @ce.
         * Including @ref StorageFlag::ReferenceCounted in @p flags causes
         * @ref LayerState::NeedsCommonDataUpdate to be set on the layer,
         * otherwise no state flags are set.
         */
        explicit AbstractStorage(DataLayer& layer, const Containers::Size3D& size, StorageFlags flags = {}): _layer{&layer}, _handle{layer.createStorage(size, flags)} {}

        /**
         * @brief Construct a single-item storage using the default @ref DataLayer on given user interface
         *
         * Like @ref AbstractStorage(DataLayer&, StorageFlags) but using the
         * default @ref DataLayer available through
         * @ref UserInterface::dataLayer(). Expects that the @p ui contains a
         * @ref DataLayer instance.
         */
        template<class UserInterface> explicit AbstractStorage(UserInterface& ui, StorageFlags flags = {}): AbstractStorage{ui, {1, 1, 1}, flags} {}

        /**
         * @brief Construct a 1D storage using the default @ref DataLayer on given user interface
         *
         * Like @ref AbstractStorage(DataLayer&, std::size_t, StorageFlags) but
         * using the default @ref DataLayer available through
         * @ref UserInterface::dataLayer(). Expects that the @p ui contains a
         * @ref DataLayer instance.
         */
        template<class UserInterface> explicit AbstractStorage(UserInterface& ui, std::size_t size, StorageFlags flags = {}): AbstractStorage{ui, {1, 1, size}, flags} {}

        /**
         * @brief Construct a 2D storage using the default @ref DataLayer on given user interface
         *
         * Like @ref AbstractStorage(DataLayer&, const Containers::Size2D&, StorageFlags)
         * but using the default @ref DataLayer available through
         * @ref UserInterface::dataLayer(). Expects that the @p ui contains a
         * @ref DataLayer instance.
         */
        template<class UserInterface> explicit AbstractStorage(UserInterface& ui, const Containers::Size2D& size, StorageFlags flags = {}): AbstractStorage{ui, {1, size[0], size[1]}, flags} {}

        /**
         * @brief Construct a 3D storage using the default @ref DataLayer on given user interface
         *
         * Like @ref AbstractStorage(DataLayer&, const Containers::Size3D&, StorageFlags)
         * but using the default @ref DataLayer available through
         * @ref UserInterface::dataLayer(). Expects that the @p ui contains a
         * @ref DataLayer instance.
         */
        template<class UserInterface> explicit AbstractStorage(UserInterface& ui, const Containers::Size3D& size, StorageFlags flags = {})
            /* GCC 15 (and likely others) in Release complain that _layer and
               _handle "may be used uninitialized" here. Explicitly zero-init
               them before filling them below to avoid this stupid warning. */
            : _layer{}, _handle{}
        {
            CORRADE_ASSERT(ui.hasDataLayer(),
                "Ui::AbstractStorage: DataLayer not present in the UI", );
            _layer = &ui.dataLayer();
            _handle = _layer->createStorage(size, flags);
        }

        /**
         * @brief Create a storage in-place
         *
         * Contrary to what Doxygen shows, returns reference to a
         * one-dimensional fixed-size array of elements, i.e.
         * @cpp T(&)[MaxInPlaceSize/sizeof(T)] @ce. There the caller is
         * expected to initialize a storage of a size specified in the
         * @ref AbstractStorage() constructor. Expects that @p T is a trivially
         * copyable type not larger than @ref MaxInPlaceSize and with alignment
         * of at most @cpp 8 @ce. If it's larger, overaligned or not trivial,
         * use @ref createAllocated() instead. If the in-place storage can stay
         * being uninitialized memory, this function doesn't need to be called
         * at all.
         */
        template<class T>
            #ifdef DOXYGEN_GENERATING_OUTPUT
            T* createInPlace()
            #else
            auto createInPlace() -> T(&)[MaxInPlaceSize/sizeof(T)]
            #endif
        {
            #ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
            static_assert(std::is_trivially_copyable<T>::value, "can only store trivially-copyable types in-place");
            #endif
            static_assert(sizeof(T) <= MaxInPlaceSize, "type too large to store in-place");
            static_assert(alignof(T) <= 8, "can't store overaligned types in-place");
            /* LOL, this cast works? */
            return reinterpret_cast<T(&)[MaxInPlaceSize/sizeof(T)]>(_layer->createStorageInPlace(_handle));
        }

        /**
         * @brief Create an allocated storage
         * @param data      Pointer to the allocated data
         * @param size      Size of the allocated data in bytes
         * @param deleter   Deleter
         * @return The @p data pointer unchanged
         *
         * When the storage is removed, @p deleter gets called with @p data and
         * @p size passed to it. The @p data and @p deleter are expected to not
         * be @cpp nullptr @ce, @p size can be zero if the deleter doesn't
         * require it. If the type is trivially copyable, aligned to at most
         * @cpp 8 @ce bytes and not larger than @ref MaxInPlaceSize, you can
         * store it in place using @ref createInPlace() and avoid the
         * allocation. It's not a requirement however.
         * @see @ref isAllocated()
         */
        template<class T> T* createAllocated(T* data, std::size_t size, void(*deleter)(void*, std::size_t)) {
            return static_cast<T*>(_layer->createStorageAllocated(_handle, data, size, deleter));
        }

        /**
         * @brief Storage data
         *
         * Returns a pointer to either the storage passed to
         * @ref createAllocated() or to the in-place storage returned by
         * @ref createInPlace(). It's the subclass responsibility to ensure
         * @p T matches the actually stored type.
         *
         * In order to have updates properly reflected to all data referencing
         * the storage, the subclass implementation should call @ref setDirty()
         * if it modifies anything in the storage. There's no way for the
         * @ref AbstractStorage implementation to detect this on its own,
         * especially considering the storage may be just a view on memory
         * managed elsewhere.
         */
        template<class T> T* data() const {
            return static_cast<T*>(_layer->storageData(_handle));
        }

    private:
        /* Mainly so DataLayer::storageInternal() can construct from a handle,
           storage<T>() then just casts to a subtype which doesn't need a
           friend on the derived type anymore */
        friend DataLayer;
        /* So it can call the private layer + handle constructor */
        template<UnsignedInt, class, class, class, class...> friend struct Implementation::StorageLambda;

        /* Used by StorageLambda and also DataLayer::storageInternal() */
        explicit AbstractStorage(DataLayer& layer, DataLayerStorageHandle handle): _layer{&layer}, _handle{handle} {}

        DataLayer* _layer;
        DataLayerStorageHandle _handle;
        /* 0/4 bytes free, but with explicit padding to make non-trivial
           subclasses fail to compile. Update when changing the members. */
        #ifndef CORRADE_TARGET_32BIT
        /* GCC 4.8 prints "warning: '*((void*)& storage +12)' may be used
           "uninitialized in this function [-Wmaybe-uninitialized]" in Release
           builds due to the anonymous padding member. We *do* want the padding
           to check at compile time that subclasses don't add any members, so
           turn it into a zero-initialized named member instead. GCC 5+ doesn't
           warn. */
        #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__ < 5
        UnsignedInt _pad{};
        #else
        UnsignedInt:32;
        #endif
        #endif
};

/**
@brief Base for @ref DataLayer storage queries
@m_since_latest_{extras}

Used through the @ref StorageQuery subclass which is returned from
@ref AbstractStorage implementations and is meant to be passed to
@ref DataLayer::onUpdate() along with an update function.
*/
class MAGNUM_UI_EXPORT AbstractStorageQuery {
    public:
        /** @brief Data layer reference */
        DataLayer& layer() const { return *_layer; }

        /**
         * @brief Storage handle
         *
         * Guaranteed to be associated with @ref layer() and never
         * @ref StorageHandle::Null.
         */
        StorageHandle storage() const {
            return storageHandle(_layer->handle(), _storage);
        }

        /** @brief Data index */
        Containers::Size3D index() const { return _index; }

        /**
         * @brief Available storage operations
         *
         * @see @ref StorageQuery::updater(), @ref StorageQuery::set(),
         *      @ref StorageQuery::reset(), @ref StorageQuery::toggle(),
         *      @ref StorageQuery::increment(), @ref StorageQuery::decrement(),
         *      @ref StorageQuery::setToMin(), @ref StorageQuery::setToMax()
         */
        StorageOperations operations() const { return _operations; }

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        /* So it can access the _call etc without having to expose those via
           getters */
        friend DataLayer;

        explicit AbstractStorageQuery(const AbstractStorage& storage, StorageOperations operations, const Containers::Size3D& index, void(*(*call)(Implementation::StorageCallOoverload))(DataLayer&, DataLayerStorageHandle, const Containers::Size3D&, Containers::FunctionData&));

        DataLayer* _layer;
        DataLayerStorageHandle _storage;
        StorageOperations _operations;
        /* 3 bytes free */
        Containers::Size3D _index;
        /* Function pointer taking StorageCallOoverload returning a void(*)(…)
           function pointer. Yes, I know. Sorry. Done this way so it's possible
           to defer a choice of particular overload until the StorageQuery is
           passed to DataLayer::onUpdate() without having to store them as
           several 8-byte pointer members. */
        void(*(*_call)(Implementation::StorageCallOoverload))(DataLayer&, DataLayerStorageHandle, const Containers::Size3D&, Containers::FunctionData&);
};

/**
@brief @ref DataLayer storage query
@m_since_latest_{extras}

Returned from @ref AbstractStorage implementations, meant to be passed to
@ref DataLayer::onUpdate() along with an update function.
*/
template<class T> class StorageQuery: public AbstractStorageQuery {
    public:
        /**
         * @brief Create a query to a single-item storage
         * @param operations    Operations supported by @p query and @p updater
         * @param storage       Storage instance
         * @param query         Lambda to call on the storage to retrieve the
         *      desired value for given operation
         * @param updater       Lambda to call to update the storage value
         *      based on supplied operation and value or @cpp nullptr @ce if
         *      the storage isn't mutable.
         *
         * Expects that @p storage is valid in the layer it's associated with
         * and is single-item, i.e. with a size of @cpp {1, 1, 1} @ce. The
         * @p query and @p updater is expected to be a non-capturing lambda
         * with given signature, *not* a function pointer.
         *
         * The @p operations are expected to be set according to
         * constraints specified in particular @ref StorageOperation values. If
         * @p operations contain @ref StorageOperation::Min /
         * @relativeref{StorageOperation,Max}, passing either of them to
         * @p query should return a min / max allowed storage value, otherwise
         * @p query is expected to return the currently stored value. If
         * @p updater is not @cpp nullptr @ce, @p operations are expected to be
         * either empty or contain @ref StorageOperation::Min /
         * @relativeref{StorageOperation,Max}. The @p updater return value
         * should follow @ref StorageUpdateState value documentation.
         */
        template<class Storage
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , class F, class G = std::nullptr_t, typename std::enable_if<std::is_convertible<F&&, T(*)(const Storage&, StorageOperation)>::value && std::is_convertible<G&&, StorageUpdateState(*)(const Storage&, StorageOperation, const T*)>::value, int>::type = 0
            #endif
        > /*implicit*/ StorageQuery(const Storage& storage,
            #ifndef DOXYGEN_GENERATING_OUTPUT
            StorageOperations operations, F query, G updater = nullptr
            #else
            StorageOperations operations, T(*query)(const Storage& storage, StorageOperation operation), StorageUpdateState(*updater)(const Storage& storage, StorageOperation operation, const T* value) = nullptr
            #endif
        );

        /**
         * @brief Create a query to a 1D storage
         * @param operations    Operations supported by @p query and @p updater
         * @param storage       Storage instance
         * @param index         Value index to query
         * @param query         Lambda to call on the storage to retrieve the
         *      desired value for given operation
         * @param updater       Lambda to call to update the storage value
         *      based on supplied operation and value or @cpp nullptr @ce if
         *      the storage isn't mutable.
         *
         * Expects that @p storage is valid in the layer it's associated with,
         * is 1D, i.e. with a size of @cpp {1, 1, size} @ce, and @p index is
         * not larger than `size`. The @p query and @p updater is expected to
         * be a non-capturing lambda with given signature, *not* a function
         * pointer.
         *
         * The @p operations are expected to be set according to
         * constraints specified in particular @ref StorageOperation values. If
         * @p operations contain @ref StorageOperation::Min /
         * @relativeref{StorageOperation,Max}, passing either of them to
         * @p query should return a min / max allowed storage value, otherwise
         * @p query is expected to return the currently stored value. If
         * @p updater is not @cpp nullptr @ce, @p operations are expected to be
         * either empty or contain @ref StorageOperation::Min /
         * @relativeref{StorageOperation,Max}. The @p updater return value
         * should follow @ref StorageUpdateState value documentation.
         */
        template<class Storage
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , class F, class G = std::nullptr_t, typename std::enable_if<std::is_convertible<F&&, T(*)(const Storage&, std::size_t, StorageOperation)>::value && std::is_convertible<G&&, StorageUpdateState(*)(const Storage&, std::size_t, StorageOperation, const T*)>::value, int>::type = 0
            #endif
        > /*implicit*/ StorageQuery(const Storage& storage, std::size_t index,
            #ifndef DOXYGEN_GENERATING_OUTPUT
            StorageOperations operations, F query, G updater = nullptr
            #else
            StorageOperations operations, T(*query)(const Storage& storage, std::size_t index, StorageOperation operation), StorageUpdateState(*updater)(const Storage& storage, std::size_t index, StorageOperation operation, const T* value) = nullptr
            #endif
        );

        /**
         * @brief Create a query to a 1D storage
         *
         * Same as @ref StorageQuery() "StorageQuery(const Storage&, std::size_t, StorageOperations, T(*)(const Storage&, std::size_t, StorageOperation), StorageUpdateState(*)(const Storage&, std::size_t, StorageOperation, const T*))",
         * just with @ref Containers::Size1D instead of @ref std::size_t as an
         * index type to make it easier for generic storage implementations.
         * @todoc fix the constructor link once Doxygen is capable of that
         */
        template<class Storage
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , class F, class G = std::nullptr_t, typename std::enable_if<std::is_convertible<F&&, T(*)(const Storage&, const Containers::Size1D&, StorageOperation)>::value && std::is_convertible<G&&, StorageUpdateState(*)(const Storage&, const Containers::Size1D&, StorageOperation, const T*)>::value, int>::type = 0
            #endif
        > /*implicit*/ StorageQuery(const Storage& storage, const Containers::Size1D& index,
            #ifndef DOXYGEN_GENERATING_OUTPUT
            StorageOperations operations, F query, G updater = nullptr
            #else
            StorageOperations operations, T(*query)(const Storage& storage, const Containers::Size1D& index, StorageOperation operation), StorageUpdateState(*updater)(const Storage& storage, const Containers::Size1D& index, StorageOperation operation, const T* value) = nullptr
            #endif
        );

        /**
         * @brief Create a query to a 2D storage
         * @param operations    Operations supported by @p query and @p updater
         * @param storage       Storage instance
         * @param index         Value index to query
         * @param query         Lambda to call on the storage to retrieve the
         *      desired value for given operation
         * @param updater       Lambda to call to update the storage value
         *      based on supplied operation and value or @cpp nullptr @ce if
         *      the storage isn't mutable.
         *
         * Expects that @p storage is valid in the layer it's associated with,
         * is 2D, i.e. with a size of @cpp {1, size[0], size[1]} @ce, and
         * @p index is not larger than `size`. The @p query and @p updater is
         * expected to be a non-capturing lambda with given signature, *not* a
         * function pointer.
         *
         * The @p operations are expected to be set according to
         * constraints specified in particular @ref StorageOperation values. If
         * @p operations contain @ref StorageOperation::Min /
         * @relativeref{StorageOperation,Max}, passing either of them to
         * @p query should return a min / max allowed storage value, otherwise
         * @p query is expected to return the currently stored value. If
         * @p updater is not @cpp nullptr @ce, @p operations are expected to be
         * either empty or contain @ref StorageOperation::Min /
         * @relativeref{StorageOperation,Max}. The @p updater return value
         * should follow @ref StorageUpdateState value documentation.
         */
        template<class Storage
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , class F, class G = std::nullptr_t, typename std::enable_if<std::is_convertible<F&&, T(*)(const Storage&, const Containers::Size2D&, StorageOperation)>::value && std::is_convertible<G&&, StorageUpdateState(*)(const Storage&, const Containers::Size2D&, StorageOperation, const T*)>::value, int>::type = 0
            #endif
        > /*implicit*/ StorageQuery(const Storage& storage, const Containers::Size2D& index,
            #ifndef DOXYGEN_GENERATING_OUTPUT
            StorageOperations operations, F query, G updater = nullptr
            #else
            StorageOperations operations, T(*query)(const Storage& storage, const Containers::Size2D& index, StorageOperation operation), StorageUpdateState(*updater)(const Storage& storage, const Containers::Size2D& index, StorageOperation operation, const T* value) = nullptr
            #endif
        );

        /**
         * @brief Create a query to a 3D storage
         * @param operations    Operations supported by @p query and @p updater
         * @param storage       Storage instance
         * @param index         Value index to query
         * @param query         Lambda to call on the storage to retrieve the
         *      desired value for given operation
         * @param updater       Lambda to call to update the storage value
         *      based on supplied operation and value or @cpp nullptr @ce if
         *      the storage isn't mutable.
         *
         * Expects that @p storage is valid in the layer it's associated with
         * and @p index is not larger than @p storage size. The @p query and
         * @p updater is expected to be a non-capturing lambda with given
         * signature, *not* a function pointer.
         *
         * The @p operations are expected to be set according to
         * constraints specified in particular @ref StorageOperation values. If
         * @p operations contain @ref StorageOperation::Min /
         * @relativeref{StorageOperation,Max}, passing either of them to
         * @p query should return a min / max allowed storage value, otherwise
         * @p query is expected to return the currently stored value. If
         * @p updater is not @cpp nullptr @ce, @p operations are expected to be
         * either empty or contain @ref StorageOperation::Min /
         * @relativeref{StorageOperation,Max}. The @p updater return value
         * should follow @ref StorageUpdateState value documentation.
         */
        template<class Storage
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , class F, class G = std::nullptr_t, typename std::enable_if<std::is_convertible<F&&, T(*)(const Storage&, const Containers::Size3D&, StorageOperation)>::value && std::is_convertible<G&&, StorageUpdateState(*)(const Storage&, const Containers::Size3D&, StorageOperation, const T*)>::value, int>::type = 0
            #endif
        > /*implicit*/ StorageQuery(const Storage& storage, const Containers::Size3D& index,
            #ifndef DOXYGEN_GENERATING_OUTPUT
            StorageOperations operations, F query, G updater = nullptr
            #else
            StorageOperations operations, T(*query)(const Storage& storage, const Containers::Size3D& index, StorageOperation operation), StorageUpdateState(*updater)(const Storage& storage, const Containers::Size3D& index, StorageOperation operation, const T* value) = nullptr
            #endif
        );

        /**
         * @brief Query implementation
         *
         * The returned function can be used to directly query the storage
         * values. If @ref operations() contain @ref StorageOperation::Min or
         * @relativeref{StorageOperation,Max}, passing these as the
         * @p operation argument will return the minimum or maximum allowed
         * value, otherwise the query returns the currently stored value.
         *
         * Unlike @ref updater(), the function is guaranteed to never be
         * @cpp nullptr @ce.
         */
        auto query() const -> T(*)(DataLayer& layer, DataLayerStorageHandle storage, const Containers::Size3D& index, StorageOperation operation) {
            return _query;
        }

        /**
         * @brief Storage value
         *
         * Returns value of given @ref storage() at @ref index(). Expects that
         * @ref storage() is still valid in the @ref layer(). Meant to be used
         * mainly for diagnostic purposes, for regular access prefer to access
         * the storage data directly.
         */
        /*implicit*/ operator T() const;

        /**
         * @brief Updater implementation
         *
         * The returned function can be used in various event callbacks to have
         * changes reflected back to the storage. If the function is
         * @cpp nullptr @ce, no updating is possible, likely because the
         * storage is immutable.
         *
         * If the function doesn't return @cpp nullptr @ce, the set of
         * operations supported by the implementation is exposed via
         * @ref operations(). If @ref operations() return an empty set, no
         * updater is available, and the pointer returned by this function may
         * likely also be @cpp nullptr @ce.
         *
         * When called, the updater updates @p storage at @p index, marking the
         * storage as dirty as appropriate. Implementations commonly expect
         * @p value to be not @cpp nullptr @ce when @p operation is
         * @ref StorageOperation::Set, it can be null in other cases. See
         * documentation of a particular storage implementation for details.
         */
        auto updater() const -> StorageUpdateState(*)(DataLayer& layer, DataLayerStorageHandle storage, const Containers::Size3D& index, StorageOperation operation, const T* value) {
            return _updater;
        }

        /**
         * @brief Update the storage value
         *
         * Expects that @ref storage() is still valid in the @ref layer() and
         * @ref operations() list @ref StorageOperation::Set. Internally calls
         * @ref updater() with @ref layer(), @ref storage(), @ref index(),
         * @ref StorageOperation::Set and a pointer to @p value, and propagates
         * its return value. See documentation of @ref updater() and
         * @ref StorageUpdateState for more information.
         * @see @ref reset(), @ref toggle()
         */
        StorageUpdateState set(const T& value) const;

        /**
         * @brief Reset the storage value
         *
         * Expects that @ref storage() is still valid in the @ref layer() and
         * @ref operations() list @ref StorageOperation::Reset. Internally
         * calls @ref updater() with @ref layer(), @ref storage(),
         * @ref index(), @ref StorageOperation::Reset and a @cpp nullptr @ce
         * value pointer. See documentation of @ref updater() for more
         * information.
         * @see @ref set(), @ref toggle()
         */
        void reset() const {
            updateInternal(
                #ifndef CORRADE_NO_ASSERT
                "Ui::StorageQuery::reset():",
                #endif
                StorageOperation::Reset);
        }

        /**
         * @brief Toggle the storage value
         *
         * Expects that @ref storage() is still valid in the @ref layer() and
         * @ref operations() list @ref StorageOperation::Toggle. Internally
         * calls @ref updater() with @ref layer(), @ref storage(),
         * @ref index(), @ref StorageOperation::Toggle and a @cpp nullptr @ce
         * value pointer. See documentation of @ref updater() for more
         * information.
         * @see @ref set(), @ref reset()
         */
        void toggle() const  {
            updateInternal(
                #ifndef CORRADE_NO_ASSERT
                "Ui::StorageQuery::toggle():",
                #endif
                StorageOperation::Toggle);
        }

        /**
         * @brief Increment the storage value
         *
         * Expects that @ref storage() is still valid in the @ref layer() and
         * @ref operations() list @ref StorageOperation::Increment. Internally
         * calls @ref updater() with @ref layer(), @ref storage(),
         * @ref index(), @ref StorageOperation::Increment and a
         * @cpp nullptr @ce value pointer. See documentation of @ref updater()
         * for more information.
         * @see @ref decrement()
         */
        void increment() const  {
            updateInternal(
                #ifndef CORRADE_NO_ASSERT
                "Ui::StorageQuery::increment():",
                #endif
                StorageOperation::Increment);
        }

        /**
         * @brief Decrement the storage value
         *
         * Expects that @ref storage() is still valid in the @ref layer() and
         * @ref operations() list @ref StorageOperation::Decrement. Internally
         * calls @ref updater() with @ref layer(), @ref storage(),
         * @ref index(), @ref StorageOperation::Decrement and a
         * @cpp nullptr @ce value pointer. See documentation of @ref updater()
         * for more information.
         * @see @ref increment()
         */
        void decrement() const {
            updateInternal(
                #ifndef CORRADE_NO_ASSERT
                "Ui::StorageQuery::decrement():",
                #endif
                StorageOperation::Decrement);
        }

        /**
         * @brief Minimum allowed storage value
         *
         * Expects that @ref storage() is still valid in the @ref layer() and
         * @ref operations() list @ref StorageOperation::Min. Internally calls
         * @ref query() with @ref layer(), @ref storage(), @ref index() and
         * @ref StorageOperation::Min. See documentation of @ref query() for
         * more information.
         * @see @ref setToMin(), @ref max()
         */
        T min() const;

        /**
         * @brief Set the storage value to a minimum
         *
         * Expects that @ref storage() is still valid in the @ref layer() and
         * @ref operations() list @ref StorageOperation::Min. Internally calls
         * @ref updater() with @ref layer(), @ref storage(), @ref index(),
         * @ref StorageOperation::Min and a @cpp nullptr @ce value pointer. See
         * documentation of @ref updater() for more information.
         * @see @ref min(), @ref setToMax()
         */
        void setToMin() const {
            updateInternal(
                #ifndef CORRADE_NO_ASSERT
                "Ui::StorageQuery::setToMin():",
                #endif
                StorageOperation::Min);
        }

        /**
         * @brief Maximum allowed storage value
         *
         * Expects that @ref storage() is still valid in the @ref layer() and
         * @ref operations() list @ref StorageOperation::Max. Internally calls
         * @ref query() with @ref layer(), @ref storage(), @ref index() and
         * @ref StorageOperation::Max. See documentation of @ref query() for
         * more information.
         * @see @ref setToMax(), @ref min()
         */
        T max() const;

        /**
         * @brief Set the storage value to a maximum
         *
         * Expects that @ref storage() is still valid in the @ref layer() and
         * @ref operations() list @ref StorageOperation::Max. Internally calls
         * @ref updater() with @ref layer(), @ref storage(), @ref index(),
         * @ref StorageOperation::Max and a @cpp nullptr @ce value pointer. See
         * documentation of @ref updater() for more information.
         * @see @ref max(), @ref setToMin()
         */
        void setToMax() const {
            updateInternal(
                #ifndef CORRADE_NO_ASSERT
                "Ui::StorageQuery::setToMax():",
                #endif
                StorageOperation::Max);
        }

    private:
        /* Delegated to from the public single-item / 1D / 2D / 3D
           constructors, contains common static assertions. The StorageArgs are
           used only because it's not possible to explicitly specify template
           arguments when calling a constructor. */
        template<UnsignedInt dimensions, class Storage, class F, class G> explicit StorageQuery(const Storage& storage, const Containers::Size3D& index, StorageOperations operations, Implementation::StorageArgs<dimensions, F, G>);
        /* Base non-templated constructor delegated to from the templated one
           above to further reduce (generated) code duplication */
        explicit StorageQuery(const AbstractStorage& storage, const Containers::Size3D& index, StorageOperations operations, void(*(*call)(Implementation::StorageCallOoverload))(DataLayer&, DataLayerStorageHandle, const Containers::Size3D&, Containers::FunctionData&), T(*const query)(DataLayer&, DataLayerStorageHandle, const Containers::Size3D&, StorageOperation), StorageUpdateState(*updater)(DataLayer&, DataLayerStorageHandle, const Containers::Size3D&, StorageOperation, const T*));

        void updateInternal(
            #ifndef CORRADE_NO_ASSERT
            const char* messagePrefix,
            #endif
            StorageOperation operation) const;

        T(*_query)(DataLayer&, DataLayerStorageHandle, const Containers::Size3D&, StorageOperation);
        StorageUpdateState(*_updater)(DataLayer&, DataLayerStorageHandle, const Containers::Size3D&, StorageOperation, const T*);
};

template<class Storage> Storage DataLayer::storage(const StorageHandle handle) {
    static_assert(
        #ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
        std::is_trivially_copyable<Storage>::value &&
        #endif
        sizeof(Storage) == sizeof(AbstractStorage),
        "AbstractStorage subclasses expected to be trivially copyable with no extra members");
    AbstractStorage storage = storageInternal(handle);
    return static_cast<const Storage&>(storage);
}

template<class Storage> Storage DataLayer::storage(const DataLayerStorageHandle handle) {
    static_assert(
        #ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
        std::is_trivially_copyable<Storage>::value &&
        #endif
        sizeof(Storage) == sizeof(AbstractStorage),
        "AbstractStorage subclasses expected to be trivially copyable with no extra members");
    AbstractStorage storage = storageInternal(handle);
    return static_cast<const Storage&>(storage);
}

namespace Implementation {
    /* Abstracts over the ability to pass lambdas containing either no index or
       std::size_t, Containers::Size1D, Size2D and Size3D index to the
       StorageQuery constructor, and in case of the updater also nullptr
       instead of a lambda */
    template<class Storage, class T, class F, class ...Args> struct StorageLambda<0, Storage, T, F, Args...> {
        static T call(DataLayer& layer, const DataLayerStorageHandle handle, const Containers::Size3D&, const StorageOperation operation, Args... args) {
            /* With a non-capturing lambda, all we need for calling it is the F
               type, there's no point in storing the (empty) `query` instance
               itself. Could also do `*reinterpret_cast<F*>(nullptr)` but
               casting from an empty struct doesn't break the "`this` pointer
               is never null" rule and avoids compiler warnings. */
            struct {} empty;
            const AbstractStorage storage{layer, handle};
            return reinterpret_cast<F&>(empty)(static_cast<const Storage&>(storage), operation, args...);
        }
    };
    template<class Storage, class T, class F, class ...Args> struct StorageLambda<1, Storage, T, F, Args...> {
        static T call(DataLayer& layer, const DataLayerStorageHandle handle, const Containers::Size3D& index, const StorageOperation operation, Args... args) {
            /* See above for why we're casting from an empty struct */
            struct {} empty;
            const AbstractStorage storage{layer, handle};
            return reinterpret_cast<F&>(empty)(static_cast<const Storage&>(storage), index[2], operation, args...);
        }
    };
    template<class Storage, class T, class F, class ...Args> struct StorageLambda<2, Storage, T, F, Args...> {
        static T call(DataLayer& layer, const DataLayerStorageHandle handle, const Containers::Size3D& index, const StorageOperation operation, Args... args) {
            /* See above for why we're casting from an empty struct */
            struct {} empty;
            const AbstractStorage storage{layer, handle};
            return reinterpret_cast<F&>(empty)(static_cast<const Storage&>(storage), {index[1], index[2]}, operation, args...);
        }
    };
    template<class Storage, class T, class F, class ...Args> struct StorageLambda<3, Storage, T, F, Args...> {
        static T call(DataLayer& layer, const DataLayerStorageHandle handle, const Containers::Size3D& index, const StorageOperation operation, Args... args) {
            /* See above for why we're casting from an empty struct */
            struct {} empty;
            const AbstractStorage storage{layer, handle};
            return reinterpret_cast<F&>(empty)(static_cast<const Storage&>(storage), index, operation, args...);
        }
    };
    /* This has to specialize on both the index and the F to be a more
       specialized match than the above variants, thus copied four times */
    template<class Storage, class T, class ...Args> struct StorageLambda<0, Storage, T, std::nullptr_t, Args...> {
        constexpr static std::nullptr_t call = nullptr;
    };
    template<class Storage, class T, class ...Args> struct StorageLambda<1, Storage, T, std::nullptr_t, Args...> {
        constexpr static std::nullptr_t call = nullptr;
    };
    template<class Storage, class T, class ...Args> struct StorageLambda<2, Storage, T, std::nullptr_t, Args...> {
        constexpr static std::nullptr_t call = nullptr;
    };
    template<class Storage, class T, class ...Args> struct StorageLambda<3, Storage, T, std::nullptr_t, Args...> {
        constexpr static std::nullptr_t call = nullptr;
    };

    /* The StorageQuery stores a pointer to a concrete instantation of this
       function, and the function then returns one of the overloads based on
       which signature is used in a particular DataLayer::onUpdate() call. Done
       this way to not have to several store pointers to all possible overloads
       in each StorageQuery instance. */
    template<UnsignedInt dimensions, class T, class Storage, class F> static auto storageCall(StorageCallOoverload overload) -> void(*)(DataLayer&, DataLayerStorageHandle, const Containers::Size3D&, Containers::FunctionData&) {
        switch(overload) {
            case StorageCallOoverload::ByReference:
                return [](DataLayer& layer, const DataLayerStorageHandle storageHandle, const Containers::Size3D& index, Containers::FunctionData& result) {
                    reinterpret_cast<Containers::Function<void(const T&)>&>(result)(StorageLambda<dimensions, Storage, T, F>::call(layer, storageHandle, index, StorageOperation{}));
                };
            case StorageCallOoverload::ByValue:
                return [](DataLayer& layer, const DataLayerStorageHandle storageHandle, const Containers::Size3D& index, Containers::FunctionData& result) {
                    reinterpret_cast<Containers::Function<void(T)>&>(result)(StorageLambda<dimensions, Storage, T, F>::call(layer, storageHandle, index, StorageOperation{}));
                };
            case StorageCallOoverload::ByValueMinMax:
                return [](DataLayer& layer, const DataLayerStorageHandle storageHandle, const Containers::Size3D& index, Containers::FunctionData& result) {
                    reinterpret_cast<Containers::Function<void(T, T, T)>&>(result)(StorageLambda<dimensions, Storage, T, F>::call(layer, storageHandle, index, StorageOperation{}), StorageLambda<dimensions, Storage, T, F>::call(layer, storageHandle, index, StorageOperation::Min), StorageLambda<dimensions, Storage, T, F>::call(layer, storageHandle, index, StorageOperation::Max));
                };
        }
        CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
    }
}

#ifndef DOXYGEN_GENERATING_OUTPUT
template<class T> template<class Storage, class F, class G, typename std::enable_if<std::is_convertible<F&&, T(*)(const Storage&, StorageOperation)>::value && std::is_convertible<G&&, StorageUpdateState(*)(const Storage&, StorageOperation, const T*)>::value, int>::type> StorageQuery<T>::StorageQuery(const Storage& storage, const StorageOperations operations, F, G): StorageQuery{storage, {}, operations, Implementation::StorageArgs<0, F, G>{}} {
    #ifndef CORRADE_NO_ASSERT
    /* Subclasses may override size to return different dimension count, query
       the base AbstractStorage tpe. The storage handle also may be invalid at
       this point (checked by the delegated-to constructor) so query the size
       only if valid. */
    const Containers::Size3D size = _layer->isHandleValid(_storage) ? static_cast<const AbstractStorage&>(storage).size() : Containers::Size3D{1, 1, 1};
    #endif
    CORRADE_ASSERT(size == (Containers::Size3D{1, 1, 1}),
        "Ui::StorageQuery: expected a single-item storage but got a size of" << size, );
}

template<class T> template<class Storage, class F, class G, typename std::enable_if<std::is_convertible<F&&, T(*)(const Storage&, std::size_t, StorageOperation)>::value && std::is_convertible<G&&, StorageUpdateState(*)(const Storage&, std::size_t, StorageOperation, const T*)>::value, int>::type> StorageQuery<T>::StorageQuery(const Storage& storage, std::size_t index, const StorageOperations operations, F, G): StorageQuery{storage, {0, 0, index}, operations, Implementation::StorageArgs<1, F, G>{}} {
    #ifndef CORRADE_NO_ASSERT
    /* Base type and handle validity reasoning same as above */
    const Containers::Size3D size = _layer->isHandleValid(_storage) ? static_cast<const AbstractStorage&>(storage).size() : Containers::Size3D{1, 1, 1};
    #endif
    CORRADE_ASSERT(size[0] == 1 && size[1] == 1,
        "Ui::StorageQuery: expected a 1D storage but got a size of" << size, );
}

template<class T> template<class Storage, class F, class G, typename std::enable_if<std::is_convertible<F&&, T(*)(const Storage&, const Containers::Size1D&, StorageOperation)>::value && std::is_convertible<G&&, StorageUpdateState(*)(const Storage&, const Containers::Size1D&, StorageOperation, const T*)>::value, int>::type> StorageQuery<T>::StorageQuery(const Storage& storage, const Containers::Size1D& index, const StorageOperations operations, F, G): StorageQuery{storage, {0, 0, index}, operations, Implementation::StorageArgs<1, F, G>{}} {
    #ifndef CORRADE_NO_ASSERT
    /* Base type and handle validity reasoning same as above */
    const Containers::Size3D size = _layer->isHandleValid(_storage) ? static_cast<const AbstractStorage&>(storage).size() : Containers::Size3D{1, 1, 1};
    #endif
    CORRADE_ASSERT(size[0] == 1 && size[1] == 1,
        "Ui::StorageQuery: expected a 1D storage but got a size of" << size, );
}

template<class T> template<class Storage, class F, class G, typename std::enable_if<std::is_convertible<F&&, T(*)(const Storage&, const Containers::Size2D&, StorageOperation)>::value && std::is_convertible<G&&, StorageUpdateState(*)(const Storage&, const Containers::Size2D&, StorageOperation, const T*)>::value, int>::type> StorageQuery<T>::StorageQuery(const Storage& storage, const Containers::Size2D& index, const StorageOperations operations, F, G): StorageQuery{storage, {0, index[0], index[1]}, operations, Implementation::StorageArgs<2, F, G>{}} {
    #ifndef CORRADE_NO_ASSERT
    /* Base type and handle validity reasoning same as above */
    const Containers::Size3D size = _layer->isHandleValid(_storage) ? static_cast<const AbstractStorage&>(storage).size() : Containers::Size3D{1, 1, 1};
    #endif
    CORRADE_ASSERT(size[0] == 1,
        "Ui::StorageQuery: expected a 2D storage but got a size of" << size, );
}

template<class T> template<class Storage, class F, class G, typename std::enable_if<std::is_convertible<F&&, T(*)(const Storage&, const Containers::Size3D&, StorageOperation)>::value && std::is_convertible<G&&, StorageUpdateState(*)(const Storage&, const Containers::Size3D&, StorageOperation, const T*)>::value, int>::type> StorageQuery<T>::StorageQuery(const Storage& storage, const Containers::Size3D& index, const StorageOperations operations, F, G): StorageQuery{storage, index, operations, Implementation::StorageArgs<3, F, G>{}} {}

template<class T> template<UnsignedInt dimensions, class Storage, class F, class G> StorageQuery<T>::StorageQuery(const Storage& storage, const Containers::Size3D& index, const StorageOperations operations, Implementation::StorageArgs<dimensions, F, G>): StorageQuery{storage, index, operations,
    Implementation::storageCall<dimensions, T, Storage, F>,
    Implementation::StorageLambda<dimensions, Storage, T, F>::call,
    Implementation::StorageLambda<dimensions, Storage, StorageUpdateState, G, const T*>::call
} {
    static_assert(
        #ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
        std::is_trivially_copyable<Storage>::value &&
        #endif
        sizeof(Storage) == sizeof(AbstractStorage),
        "AbstractStorage subclasses expected to be trivially copyable with no extra members");
    static_assert(std::is_empty<F>::value,
        "expected query to be a non-capturing lambda");
    static_assert(std::is_same<G, std::nullptr_t>::value || std::is_empty<G>::value,
        "expected updater to be a nullptr or a non-capturing lambda");
}

template<class T> StorageQuery<T>::StorageQuery(const AbstractStorage& storage, const Containers::Size3D& index, const StorageOperations operations, void(*(*const call)(Implementation::StorageCallOoverload))(DataLayer&, DataLayerStorageHandle, const Containers::Size3D&, Containers::FunctionData&), T(*const query)(DataLayer&, DataLayerStorageHandle, const Containers::Size3D&, StorageOperation), StorageUpdateState(*const updater)(DataLayer&, DataLayerStorageHandle, const Containers::Size3D&, StorageOperation, const T*)): AbstractStorageQuery{storage, operations, index, call}, _query{query}, _updater{updater} {
    /* Min|Max can be used for the query as well */
    CORRADE_ASSERT(!(operations & ~(StorageOperation::Min|StorageOperation::Max)) || updater,
        "Ui::StorageQuery:" << (operations & ~(StorageOperation::Min|StorageOperation::Max)) << "requires a non-null updater", );
}

template<class T> StorageQuery<T>::operator T() const {
    /* Calling into _query even from asserts to have this compile even with a
       non-default-constructible T */
    CORRADE_ASSERT(_layer->isHandleValid(_storage),
        "Ui::StorageQuery: invalid handle" << storageHandle(_layer->handle(), _storage),
        _query(*_layer, _storage, _index, StorageOperation{}));
    return _query(*_layer, _storage, _index, StorageOperation{});
}

template<class T> StorageUpdateState StorageQuery<T>::set(const T& value) const {
    CORRADE_ASSERT(_layer->isHandleValid(_storage),
        "Ui::StorageQuery::set(): invalid handle" << storageHandle(_layer->handle(), _storage), {});
    CORRADE_ASSERT(_operations >= StorageOperation::Set,
        "Ui::StorageQuery::set():" << StorageOperation::Set << "not supported", {});
    return _updater(*_layer, _storage, _index, StorageOperation::Set, &value);
}

template<class T> void StorageQuery<T>::updateInternal(
    #ifndef CORRADE_NO_ASSERT
    const char* const messagePrefix,
    #endif
    const StorageOperation operation) const
{
    CORRADE_ASSERT(_layer->isHandleValid(_storage),
        messagePrefix << "invalid handle" << storageHandle(_layer->handle(), _storage), );
    CORRADE_ASSERT(_operations >= operation,
        messagePrefix << operation << "not supported", );
    #ifndef CORRADE_NO_ASSERT
    const StorageUpdateState state =
    #endif
        _updater(*_layer, _storage, _index, operation, nullptr);
    CORRADE_ASSERT(state == StorageUpdateState::Success,
        messagePrefix << "updater implementation expected to return" << StorageUpdateState::Success << "for" << operation << "but got" << state, );
}

template<class T> T StorageQuery<T>::min() const {
    /* Calling into _query even from asserts to have this compile even with a
       non-default-constructible T */
    CORRADE_ASSERT(_layer->isHandleValid(_storage),
        "Ui::StorageQuery::min(): invalid handle" << storageHandle(_layer->handle(), _storage),
        _query(*_layer, _storage, _index, StorageOperation::Min));
    CORRADE_ASSERT(_operations >= StorageOperation::Min,
        "Ui::StorageQuery::min():" << StorageOperation::Min << "not supported",
        _query(*_layer, _storage, _index, StorageOperation::Min));
    return _query(*_layer, _storage, _index, StorageOperation::Min);
}

template<class T> T StorageQuery<T>::max() const {
    /* Calling into _query even from asserts to have this compile even with a
       non-default-constructible T */
    CORRADE_ASSERT(_layer->isHandleValid(_storage),
        "Ui::StorageQuery::max(): invalid handle" << storageHandle(_layer->handle(), _storage),
        _query(*_layer, _storage, _index, StorageOperation::Max));
    CORRADE_ASSERT(_operations >= StorageOperation::Max,
        "Ui::StorageQuery::max():" << StorageOperation::Max << "not supported",
        _query(*_layer, _storage, _index, StorageOperation::Max));
    return _query(*_layer, _storage, _index, StorageOperation::Max);
}
#endif

}}

#endif
