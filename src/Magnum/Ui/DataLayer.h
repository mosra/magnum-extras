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
 * @brief Class @ref Magnum::Ui::DataLayer, @ref Magnum::Ui::AbstractStorage, handle @ref Magnum::Ui::DataLayerStorageHandle, @ref Magnum::Ui::StorageHandle, enum @ref Magnum::Ui::StorageFlag, enum set @ref Magnum::Ui::StorageFlags, function @ref Magnum::Ui::dataLayerStorageHandle(), @ref Magnum::Ui::dataLayerStorageHandleId(), @ref Magnum::Ui::dataLayerStorageHandleGeneration(), @ref Magnum::Ui::storageHandle(), @ref Magnum::Ui::storageHandleLayer(), @ref Magnum::Ui::storageHandleStorage(), @ref Magnum::Ui::storageHandleLayerId(), @ref Magnum::Ui::storageHandleLayerGeneration(), @ref Magnum::Ui::storageHandleId(), @ref Magnum::Ui::storageHandleGeneration()
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

namespace Implementation {
    enum: std::size_t {
        /* Used by AbstractStorage interfaces as well as DataLayer internals */
        #ifndef CORRADE_TARGET_32BIT
        DataLayerStorageMaxInPlaceSize = 4*sizeof(std::size_t)
        #else
        DataLayerStorageMaxInPlaceSize = 5*sizeof(std::size_t)
        #endif
    };
    /* The only way to pass type-erased AbstractStorage subclass function
       pointers around is to reinterpret them as plain data of sufficient size.
       Attempting to cast them to void(AbstractStorage::*)() and similar causes
       "cast between incompatible pointer to member types" warning on GCC, and
       with MSVC having certain function pointers of different size doing so
       would be playing with fire. Containers::Function does the same, reusing
       the size constant it has, and having static_assert() in all create()
       overloads to ensure it's sufficiently large. Also using a std::size_t
       array instead of plain chars to have a 4/8-byte alignment, as that's
       likely important. */
    using DataLayerTypeErasedFunctionPointer = std::size_t[Containers::Implementation::FunctionPointerSize];
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
         * is valid.
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
         * incremented after every @ref create() call referencing given
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
         * @brief Create a data binding for a single-item storage
         * @param storage       Storage to get the data from
         * @param update        Function to call when data get updated
         * @param node          Node to attach to
         * @return New data handle
         *
         * Expects that @p storage is a valid storage from this layer, it's
         * single-item, i.e. with a size of @cpp {1, 1, 1} @ce, and @p update
         * is not @cpp nullptr @ce. The @p update gets called with the return
         * value of @cpp operator*() const @ce on @p storage during
         * @ref AbstractUserInterface::update() or
         * @relativeref{AbstractUserInterface,draw()} right after the data
         * binding was created, and then every time @p storage is marked as
         * dirty.
         *
         * Use the @ref create() "create(const Storage&, T(Storage::*)() const, Containers::Function<void(T)>&&, NodeHandle)"
         * overload to query a concrete member function and the
         * @ref create(const Storage&, std::size_t, Containers::Function<void(T)>&&, NodeHandle),
         * @ref create(const Storage&, const Containers::Size2D&, Containers::Function<void(T)>&&, NodeHandle)
         * and @ref create(const Storage&, const Containers::Size3D&, Containers::Function<void(T)>&&, NodeHandle)
         * overloads to pick value at a concrete index from 1D, 2D and 3D
         * storage implementations.
         *
         * Delegates to @ref AbstractLayer::create(), see its documentation for
         * detailed description of all constraints. Calling this function
         * causes @ref LayerState::NeedsCommonDataUpdate to be set.
         * @todoc fix the create() links once Doxygen stops being shit
         * @see @ref isStorageDirty()
         */
        template<class Storage> DataHandle create(const Storage& storage,
            #ifdef DOXYGEN_GENERATING_OUTPUT
            Containers::Function<void(T)>&& /* For a less confusing signature */
            #elif defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__ < 5
            /* GCC 4.8 fails to match T without the std::common_type. GCC 5+
               works. Similar trick is used in the create() overloads with an
               explicit member, where it's needed for all compilers. */
            typename std::common_type<Containers::Function<void(decltype(*std::declval<Storage>()))>>::type&&
            #else
            Containers::Function<void(decltype(*std::declval<Storage>()))>&&
            #endif
        update, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            /** @todo Calling the operator directly from the updateCall lambda
                instead of through a pointer may be faster and allow further inlining. Benchmark. Similarly for the 1D/2D/3D overloads. */
            return create<Storage>(storage, &Storage::operator*, Utility::move(update), node);
        }

        /**
         * @brief Create a data binding for a concrete member of a single-item storage
         * @param storage       Storage to get the data from
         * @param member        Storage member to query
         * @param update        Function to call when data get updated
         * @param node          Node to attach to
         * @return New data handle
         *
         * Expects that @p storage is a valid storage from this layer, it's
         * single-item, i.e. with a size of @cpp {1, 1, 1} @ce, and @p member
         * and @p update are not @cpp nullptr @ce. The @p update gets called
         * with the return value of @p member on @p storage during
         * @ref AbstractUserInterface::update() or
         * @relativeref{AbstractUserInterface,draw()} right after the data
         * binding was created, and then every time @p storage is marked as
         * dirty.
         *
         * Use the @ref create() "create(const Storage&, T(Storage::*)(std::size_t) const, std::size_t, Containers::Function<void(T)>&&, NodeHandle)",
         * @ref create() "create(const Storage&&, T(Storage::*)(const Containers::Size2D&) const, const Containers::Size2D&, Containers::Function<void(T)>&&, NodeHandle)"
         * and @ref create() "create(const Storage&&, T(Storage::*)(const Containers::Size3D&) const, const Containers::Size3D&, Containers::Function<void(T)>&&, NodeHandle)"
         * overloads to pick value at a concrete index from 1D, 2D and 3D storage
         * implementations.
         *
         * Delegates to @ref AbstractLayer::create(), see its documentation for
         * detailed description of all constraints. Calling this function
         * causes @ref LayerState::NeedsCommonDataUpdate to be set.
         * @todoc fix the create() links once Doxygen stops being shit
         * @see @ref isStorageDirty()
         */
        template<class Storage, class T> DataHandle create(const Storage& storage, T(Storage::*member)() const,
            #ifdef DOXYGEN_GENERATING_OUTPUT
            Containers::Function<void(T)>&&
            #else /* Without this, deduction of T won't work */
            typename std::common_type<Containers::Function<void(T)>>::type&&
            #endif
        update, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        );

        /**
         * @brief Create a data binding for a 1D storage
         * @param storage       Storage to get the data from
         * @param index         Value index to query
         * @param update        Function to call when data get updated
         * @param node          Node to attach to
         * @return New data handle
         *
         * Expects that @p storage is a valid storage from this layer, it's 1D,
         * i.e. with a size of @cpp {1, 1, size} @ce, @p index is not larger
         * than `size`, and @p update is not @cpp nullptr @ce. The @p update
         * gets called with the return value of
         * @cpp operator[](std::size_t) const @ce with given @p index on
         * @p storage during @ref AbstractUserInterface::update() or
         * @relativeref{AbstractUserInterface,draw()} right after the data
         * binding was created, and then every time @p storage is marked as
         * dirty.
         *
         * Use the @ref create() "create(const Storage&, T(Storage::*)(std::size_t) const, std::size_t, Containers::Function<void(T)>&&, NodeHandle)"
         * overload to query a concrete member function.
         *
         * Delegates to @ref AbstractLayer::create(), see its documentation for
         * detailed description of all constraints. Calling this function
         * causes @ref LayerState::NeedsCommonDataUpdate to be set.
         * @todoc fix the create() links once Doxygen stops being shit
         * @see @ref isStorageDirty(), @ref storageSize()
         */
        template<class Storage> DataHandle create(const Storage& storage, std::size_t index,
            #ifdef DOXYGEN_GENERATING_OUTPUT
            Containers::Function<void(T)>&& /* For a less confusing signature */
            #elif defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__ < 5
            /* GCC 4.8 fails to match T w/o the std::common_type, see above */
            typename std::common_type<Containers::Function<void(decltype(std::declval<Storage>()[std::size_t{}]))>>::type&&
            #else
            Containers::Function<void(decltype(std::declval<Storage>()[std::size_t{}]))>&&
            #endif
        update, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return create<Storage>(storage, &Storage::operator[], index, Utility::move(update), node);
        }

        /**
         * @brief Create a data binding for a concrete member of a 1D storage
         * @param storage       Storage to get the data from
         * @param member        Storage member to query
         * @param index         Value index to query
         * @param update        Function to call when data get updated
         * @param node          Node to attach to
         * @return New data handle
         *
         * Expects that @p storage is a valid storage from this layer, it's 1D,
         * i.e. with a size of @cpp {1, 1, size} @ce, @p index is not larger
         * than `size`, and @p member and @p update are not @cpp nullptr @ce.
         * The @p update gets called with the return value of @p member with
         * given @p index on @p storage during
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
        template<class Storage, class T> DataHandle create(const Storage& storage, T(Storage::*member)(std::size_t) const, std::size_t index,
            #ifdef DOXYGEN_GENERATING_OUTPUT
            Containers::Function<void(T)>&&
            #else /* Without this, deduction of T won't work */
            typename std::common_type<Containers::Function<void(T)>>::type&&
            #endif
        update, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        );

        /**
         * @brief Create a data binding for a 2D storage
         * @param storage       Storage to get the data from
         * @param index         Value index to query
         * @param update        Function to call when data get updated
         * @param node          Node to attach to
         * @return New data handle
         *
         * Expects that @p storage is a valid storage from this layer, it's 2D,
         * i.e. with a size of @cpp {1, size[0], size[1]} @ce, @p index is not
         * larger than `size`, and @p update is not @cpp nullptr @ce. The
         * @p update gets called with the return value of
         * @cpp operator[](const Containers::Size2D&) const @ce with given
         * @p index on @p storage during @ref AbstractUserInterface::update()
         * or @relativeref{AbstractUserInterface,draw()} right after the data
         * binding was created, and then every time @p storage is marked as
         * dirty.
         *
         * Use the @ref create() "create(const Storage&, T(Storage::*)(std::size_t) const, std::size_t, Containers::Function<void(T)>&&, NodeHandle)"
         * overload to query a concrete member function.
         *
         * Delegates to @ref AbstractLayer::create(), see its documentation for
         * detailed description of all constraints. Calling this function
         * causes @ref LayerState::NeedsCommonDataUpdate to be set.
         * @todoc fix the create() links once Doxygen stops being shit
         * @see @ref isStorageDirty(), @ref storageSize()
         */
        template<class Storage> DataHandle create(const Storage& storage, const Containers::Size2D& index,
            #ifdef DOXYGEN_GENERATING_OUTPUT
            Containers::Function<void(T)>&& /* For a less confusing signature */
            #elif defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__ < 5
            /* GCC 4.8 fails to match T w/o the std::common_type, see above */
            typename std::common_type<Containers::Function<void(decltype(std::declval<Storage>()[Containers::Size2D{}]))>>::type&&
            #else
            Containers::Function<void(decltype(std::declval<Storage>()[Containers::Size2D{}]))>&&
            #endif
        update, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return create<Storage>(storage, &Storage::operator[], index, Utility::move(update), node);
        }

        /**
         * @brief Create a data binding for a concrete member of a 2D storage
         * @param storage       Storage to get the data from
         * @param member        Storage member to query
         * @param index         Value index to query
         * @param update        Function to call when data get updated
         * @param node          Node to attach to
         * @return New data handle
         *
         * Expects that @p storage is a valid storage from this layer, it's 2D,
         * i.e. with a size of @cpp {1, size[0], size[1]} @ce, @p index is not
         * larger than `size`, and @p member and @p update are not
         * @cpp nullptr @ce. The @p update gets called with the return value of
         * @p member with given @p index on @p storage during
         * @ref AbstractUserInterface::update() or
         * @relativeref{AbstractUserInterface,draw()} right after the data
         * binding was created, and then every time @p storage is marked as
         * dirty.
         *
         * Delegates to @ref AbstractLayer::create(), see its documentation for
         * detailed description of all constraints. Calling this function
         * causes @ref LayerState::NeedsCommonDataUpdate to be set.
         * @see @ref isStorageDirty(), @ref storageSize()
         */
        template<class Storage, class T> DataHandle create(const Storage& storage, T(Storage::*member)(const Containers::Size2D&) const, const Containers::Size2D& index,
            #ifdef DOXYGEN_GENERATING_OUTPUT
            Containers::Function<void(T)>&&
            #else /* Without this, deduction of T won't work */
            typename std::common_type<Containers::Function<void(T)>>::type&&
            #endif
        update, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        );

        /**
         * @brief Create a data binding for a 3D storage
         * @param storage       Storage to get the data from
         * @param index         Value index to query
         * @param update        Function to call when data get updated
         * @param node          Node to attach to
         * @return New data handle
         *
         * Expects that @p storage is a valid storage from this layer, @p index
         * is not larger than @p storage size, and @p update is not
         * @cpp nullptr @ce. The @p update gets called with the return value of
         * @cpp operator[](const Containers::Size3D&) const @ce with given
         * @p index on @p storage during @ref AbstractUserInterface::update()
         * or @relativeref{AbstractUserInterface,draw()} right after the data
         * binding was created, and then every time @p storage is marked as
         * dirty.
         *
         * Use the @ref create() "create(const Storage&, T(Storage::*)(std::size_t) const, std::size_t, Containers::Function<void(T)>&&, NodeHandle)"
         * overload to query a concrete member function.
         *
         * Delegates to @ref AbstractLayer::create(), see its documentation for
         * detailed description of all constraints. Calling this function
         * causes @ref LayerState::NeedsCommonDataUpdate to be set.
         * @todoc fix the create() links once Doxygen stops being shit
         * @see @ref isStorageDirty(), @ref storageSize()
         */
        template<class Storage> DataHandle create(const Storage& storage, const Containers::Size3D& index,
            #ifdef DOXYGEN_GENERATING_OUTPUT
            Containers::Function<void(T)>&& /* For a less confusing signature */
            #elif defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__ < 5
            /* GCC 4.8 fails to match T w/o the std::common_type, see above */
            typename std::common_type<Containers::Function<void(decltype(std::declval<Storage>()[Containers::Size3D{}]))>>::type&&
            #else
            Containers::Function<void(decltype(std::declval<Storage>()[Containers::Size3D{}]))>&&
            #endif
        update, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return create<Storage>(storage, &Storage::operator[], index, Utility::move(update), node);
        }

        /**
         * @brief Create a data binding for a concrete member of a 3D storage
         * @param storage       Storage to get the data from
         * @param member        Storage member to query
         * @param index         Value index to query
         * @param update        Function to call when data get updated
         * @param node          Node to attach to
         * @return New data handle
         *
         * Expects that @p storage is a valid storage from this layer, @p index
         * is not larger than @p storage size, and @p member and @p update are
         * not @cpp nullptr @ce. The @p update gets called with the return
         * value of @p member with given @p index on @p storage during
         * @ref AbstractUserInterface::update() or
         * @relativeref{AbstractUserInterface,draw()} right after the data
         * binding was created, and then every time @p storage is marked as
         * dirty.
         *
         * Delegates to @ref AbstractLayer::create(), see its documentation for
         * detailed description of all constraints. Calling this function
         * causes @ref LayerState::NeedsCommonDataUpdate to be set.
         * @see @ref isStorageDirty(), @ref storageSize()
         */
        template<class Storage, class T> DataHandle create(const Storage& storage, T(Storage::*member)(const Containers::Size3D&) const, const Containers::Size3D& index,
            #ifdef DOXYGEN_GENERATING_OUTPUT
            Containers::Function<void(T)>&&
            #else /* Without this, deduction of T won't work */
            typename std::common_type<Containers::Function<void(T)>>::type&&
            #endif
        update, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        );

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

        /* The layer argument in these four is just for the assert, but since
           it has to be an exported symbol to be used from the templated
           create() functions, it cannot be wrapped in #ifdef CORRADE_NO_ASSERT
           because it'd cause linker errors if the library is built with
           assertions but the user project not and vice versa. */
        DataHandle createInternal(const DataLayer& layer, DataLayerStorageHandle storage, const Implementation::DataLayerTypeErasedFunctionPointer& member, void(*updateCall)(const AbstractStorage&, const Implementation::DataLayerTypeErasedFunctionPointer&, const Containers::Size3D&, Containers::FunctionData&), Containers::FunctionData&& update, NodeHandle node);
        DataHandle createInternal(const DataLayer& layer, DataLayerStorageHandle storage, const Implementation::DataLayerTypeErasedFunctionPointer& member, void(*updateCall)(const AbstractStorage&, const Implementation::DataLayerTypeErasedFunctionPointer&, const Containers::Size3D&, Containers::FunctionData&), std::size_t index, Containers::FunctionData&& update, NodeHandle node);
        DataHandle createInternal(const DataLayer& layer, DataLayerStorageHandle storage, const Implementation::DataLayerTypeErasedFunctionPointer& member, void(*updateCall)(const AbstractStorage&, const Implementation::DataLayerTypeErasedFunctionPointer&, const Containers::Size3D&, Containers::FunctionData&), const Containers::Size2D& index, Containers::FunctionData&& update, NodeHandle node);
        DataHandle createInternal(const DataLayer& layer, DataLayerStorageHandle storage, const Implementation::DataLayerTypeErasedFunctionPointer& member, void(*updateCall)(const AbstractStorage&, const Implementation::DataLayerTypeErasedFunctionPointer&, const Containers::Size3D&, Containers::FunctionData&), const Containers::Size3D& index, Containers::FunctionData&& update, NodeHandle node);
        MAGNUM_UI_LOCAL void removeInternal(UnsignedInt id);
        MAGNUM_UI_LOCAL StorageHandle storageInternal(const UnsignedInt id) const;
        MAGNUM_UI_LOCAL Containers::Size3D indexInternal(const UnsignedInt id) const;
        MAGNUM_UI_LOCAL void setIndexInternal(const UnsignedInt id, std::size_t index);
        MAGNUM_UI_LOCAL void setIndexInternal(const UnsignedInt id, const Containers::Size2D& index);
        MAGNUM_UI_LOCAL void setIndexInternal(const UnsignedInt id, const Containers::Size3D& index);

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
        void createAllocated(void* data, std::size_t size, void(*deleter)(void*, std::size_t)) {
            _layer->createStorageAllocated(_handle, data, size, deleter);
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

        /* Used by DataLayer::storageInternal() as well as doPreUpdate() for
           creating temporary instances to pass to the update functions */
        explicit AbstractStorage(DataLayer& layer, DataLayerStorageHandle handle):
        _layer{&layer}, _handle{handle} {}

        DataLayer* _layer;
        DataLayerStorageHandle _handle;
        /* 0/4 bytes free */
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

#ifndef DOXYGEN_GENERATING_OUTPUT
template<class Storage, class T> DataHandle DataLayer::create(const Storage& storage, T(Storage::*const member)() const, typename std::common_type<Containers::Function<void(T)>>::type&& update, const NodeHandle node) {
    static_assert(
        #ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
        std::is_trivially_copyable<Storage>::value &&
        #endif
        sizeof(Storage) == sizeof(AbstractStorage),
        "AbstractStorage subclasses expected to be trivially copyable with no extra members");
    static_assert(sizeof(member) <= sizeof(Implementation::DataLayerTypeErasedFunctionPointer),
        "size of member function pointer is incorrectly assumed to be smaller");
    CORRADE_ASSERT(member,
        "Ui::DataLayer::create(): member is null", {});
    return createInternal(*storage._layer, storage._handle,
        reinterpret_cast<const Implementation::DataLayerTypeErasedFunctionPointer&>(member),
        [](const AbstractStorage& storage, const Implementation::DataLayerTypeErasedFunctionPointer& member, const Containers::Size3D&, Containers::FunctionData& update) {
            reinterpret_cast<Containers::Function<void(T)>&>(update)((static_cast<const Storage&>(storage).*reinterpret_cast<T(Storage::*const&)() const>(member))());
        },
        Utility::move(update), node);
}

template<class Storage, class T> DataHandle DataLayer::create(const Storage& storage, T(Storage::*const member)(std::size_t) const, const std::size_t index, typename std::common_type<Containers::Function<void(T)>>::type&& update, const NodeHandle node) {
    static_assert(
        #ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
        std::is_trivially_copyable<Storage>::value &&
        #endif
        sizeof(Storage) == sizeof(AbstractStorage),
        "AbstractStorage subclasses expected to be trivially copyable with no extra members");
    static_assert(sizeof(member) <= sizeof(Implementation::DataLayerTypeErasedFunctionPointer),
        "size of member function pointer is incorrectly assumed to be smaller");
    CORRADE_ASSERT(member,
        "Ui::DataLayer::create(): member is null", {});
    return createInternal(*storage._layer, storage._handle,
        reinterpret_cast<const Implementation::DataLayerTypeErasedFunctionPointer&>(member),
        [](const AbstractStorage& storage, const Implementation::DataLayerTypeErasedFunctionPointer& member, const Containers::Size3D& index, Containers::FunctionData& update) {
            reinterpret_cast<Containers::Function<void(T)>&>(update)(((static_cast<const Storage&>(storage).*reinterpret_cast<T(Storage::*const&)(std::size_t) const>(member))(index[2])));
        },
        index, Utility::move(update), node);
}

template<class Storage, class T> DataHandle DataLayer::create(const Storage& storage, T(Storage::*const member)(const Containers::Size2D&) const, const Containers::Size2D& index, typename std::common_type<Containers::Function<void(T)>>::type&& update, const NodeHandle node) {
    static_assert(
        #ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
        std::is_trivially_copyable<Storage>::value &&
        #endif
        sizeof(Storage) == sizeof(AbstractStorage),
        "AbstractStorage subclasses expected to be trivially copyable with no extra members");
    static_assert(sizeof(member) <= sizeof(Implementation::DataLayerTypeErasedFunctionPointer),
        "size of member function pointer is incorrectly assumed to be smaller");
    CORRADE_ASSERT(member,
        "Ui::DataLayer::create(): member is null", {});
    return createInternal(*storage._layer, storage._handle,
        reinterpret_cast<const Implementation::DataLayerTypeErasedFunctionPointer&>(member),
        [](const AbstractStorage& storage, const Implementation::DataLayerTypeErasedFunctionPointer& member, const Containers::Size3D& index, Containers::FunctionData& update) {
            reinterpret_cast<Containers::Function<void(T)>&>(update)((static_cast<const Storage&>(storage).*reinterpret_cast<T(Storage::*const&)(const Containers::Size2D&) const>(member))({index[1], index[2]}));
        },
        index, Utility::move(update), node);
}

template<class Storage, class T> DataHandle DataLayer::create(const Storage& storage, T(Storage::*const member)(const Containers::Size3D&) const, const Containers::Size3D& index, typename std::common_type<Containers::Function<void(T)>>::type&& update, const NodeHandle node) {
    static_assert(
        #ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
        std::is_trivially_copyable<Storage>::value &&
        #endif
        sizeof(Storage) == sizeof(AbstractStorage),
        "AbstractStorage subclasses expected to be trivially copyable with no extra members");
    static_assert(sizeof(member) <= sizeof(Implementation::DataLayerTypeErasedFunctionPointer),
        "size of member function pointer is incorrectly assumed to be smaller");
    CORRADE_ASSERT(member,
        "Ui::DataLayer::create(): member is null", {});
    return createInternal(*storage._layer, storage._handle,
        reinterpret_cast<const Implementation::DataLayerTypeErasedFunctionPointer&>(member),
        [](const AbstractStorage& storage, const Implementation::DataLayerTypeErasedFunctionPointer& member, const Containers::Size3D& index, Containers::FunctionData& update) {
            reinterpret_cast<Containers::Function<void(T)>&>(update)((static_cast<const Storage&>(storage).*reinterpret_cast<T(Storage::*const&)(const Containers::Size3D&) const>(member))(index));
        },
        index, Utility::move(update), node);
}
#endif

}}

#endif
