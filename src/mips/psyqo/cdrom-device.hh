/*

MIT License

Copyright (c) 2022 PCSX-Redux authors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#pragma once

#include <EASTL/fixed_vector.h>
#include <EASTL/functional.h>
#include <stdint.h>

#include <cstdint>
#include <type_traits>

#include "psyqo/cdrom.hh"
#include "psyqo/msf.hh"
#include "psyqo/task.hh"

namespace psyqo {

class GPU;

namespace Concepts {

template <typename T, typename = void>
struct CDRomDeviceStateEnumHasIdle : std::false_type {};

template <typename T>
struct CDRomDeviceStateEnumHasIdle<T, std::enable_if_t<T::IDLE == T(0)>> : std::true_type {};

template <typename T>
concept IsCDRomDeviceStateEnum =
    std::is_enum_v<T> && std::is_same_v<uint8_t, std::underlying_type_t<T>> && CDRomDeviceStateEnumHasIdle<T>::value;

}  // namespace Concepts

/**
 * @brief A specialization of the CDRom interface.
 *
 * @details This class is a specialization of the CDRom interface, which
 * provides a way to read from the physical CDRom drive of the console.
 * All of the methods in this class are asynchronous, and will call the
 * provided callback when the operation is complete. The class also
 * provides a blocking variant for some of the methods, which can be
 * used to perform the operation synchronously. Note that the blocking
 * variants are only provided for methods that are expected to complete
 * quickly, and should not be used in performance-critical code, as they
 * can still block the system for several milliseconds. The callbacks
 * will be called from the main thread, and have a boolean parameter
 * that indicates whether the operation was successful.
 *
 */
class CDRomDevice final : public CDRom {
  public:
    typedef eastl::fixed_vector<uint8_t, 16, false> Response;

  private:
    struct ActionBase {
        const char *name() const { return m_name; }

      protected:
        ActionBase(const char *const name) : m_name(name) {}
        virtual ~ActionBase() = default;

        virtual bool dataReady(const Response &response);
        virtual bool complete(const Response &response);
        virtual bool acknowledge(const Response &response);
        virtual bool end(const Response &response);

        void setCallback(eastl::function<void(bool)> &&callback);
        void setSuccess(bool success);

        friend class CDRomDevice;
        CDRomDevice *m_device = nullptr;
        const char *const m_name = nullptr;
    };

  public:
    virtual ~CDRomDevice();

    /**
     * @brief Prepares the CDRom subsystem.
     *
     * @details This method prepares the kernel and the system for the
     * CDRom subsystem. It should be called once from the application's
     * `prepare` method.
     *
     */
    void prepare();

    /**
     * @brief Resets the CDRom controller.
     *
     * @details This method will reset the CDRom controller. It technically
     * does not need to be called, but it is a good idea to call it when
     * the application starts, in order to ensure that the controller
     * is in a known state.
     *
     */
    void reset(eastl::function<void(bool)> &&callback);
    TaskQueue::Task scheduleReset();

    /**
     * @brief Reads sectors from the CDRom.
     *
     * @details This method will read a number of sectors from the CDRom
     * drive. The sectors will be read into the provided buffer. Note that
     * only one read operation can be active at a time, and that the
     * `ISO9660Parser` class will call this method to read the filesystem
     * structure, so care must be taken to ensure no other read operation
     * is active when the parser is used.
     *
     * @param sector The sector to start reading from.
     * @param count The number of sectors to read.
     * @param buffer The buffer to read the sectors into.
     * @param callback The callback to call when the read is complete.
     *
     */
    void readSectors(uint32_t sector, uint32_t count, void *buffer, eastl::function<void(bool)> &&callback) override;
    TaskQueue::Task scheduleReadSectors(uint32_t sector, uint32_t count, void *buffer);

    /**
     * @brief Gets the size of the Table of Contents from the CDRom. Note that
     * while the blocking variant is available because it is a fairly short
     * operation with the CDRom controller, it can still block the system
     * for roughly 2ms, which is a long time in the context of a 33MHz CPU.
     *
     * @param size The pointer to store the size of the TOC.
     * @param callback The callback to call when the size is retrieved.
     */
    void getTOCSize(unsigned *size, eastl::function<void(bool)> &&callback);
    TaskQueue::Task scheduleGetTOCSize(unsigned *size);
    unsigned getTOCSizeBlocking(GPU &);

    /**
     * @brief Reads the Table of Contents from the CDRom.
     *
     * @details This method will read the Table of Contents from the CDRom
     * drive. The TOC will be read into the provided buffer. Note that
     * a CD-Rom can have up to 99 tracks, and the TOC will be read into the
     * provided buffer starting at index 1 for the first track. Any tracks
     * that are not present on the CD will not have their MSF structure
     * filled in, so the application should ensure that the buffer is
     * initialized to zero before calling this method. The blocking variant
     * may take a total of 200ms to complete, depending on the number of
     * tracks on the CD.
     *
     * @param toc The buffer to read the TOC into.
     * @param size The size of the buffer. Should be 100 to hold all possible tracks.
     * @param callback The callback to call when the read is complete.
     */
    void readTOC(MSF *toc, unsigned size, eastl::function<void(bool)> &&callback);
    TaskQueue::Task scheduleReadTOC(MSF *toc, unsigned size);
    bool readTOCBlocking(MSF *toc, unsigned size, GPU &);

    /**
     * @brief Mutes the CD audio for both CDDA and CDXA.
     *
     * @param callback The callback to call when the mute operation is complete.
     */
    void mute(eastl::function<void(bool)> &&callback);
    TaskQueue::Task scheduleMute();
    void muteBlocking(GPU &);

    /**
     * @brief Unmutes the CD audio for both CDDA and CDXA.
     *
     * @param callback The callback to call when the unmute operation is complete.
     */
    void unmute(eastl::function<void(bool)> &&callback);
    TaskQueue::Task scheduleUnmute();
    void unmuteBlocking(GPU &);

    /**
     * @brief Begins playing CDDA audio from a given starting point.
     *
     * @details This method will begin playing CDDA audio from a given
     * starting point. The starting point is either a track number or
     * an MSF value. The callback will be called when playback is complete,
     * paused, or if an error occurs, which can be after the end of the
     * track if `stopAtEndOfTrack` is set to true, or at the end of the
     * disc if the last track is reached.
     *
     * @param start The starting point for playback.
     * @param stopAtEndOfTrack If true, playback will stop at the end of the track.
     * @param callback The callback to call when playback is complete.
     */
    void playCDDA(MSF start, bool stopAtEndOfTrack, eastl::function<void(bool)> &&callback);
    void playCDDA(unsigned track, bool stopAtEndOfTrack, eastl::function<void(bool)> &&callback);

    /**
     * @brief The action base class for the internal state machine.
     *
     * @details This class is meant to be extended by the various actions
     * that the CDRom device can perform. It provides a framework for
     * the state machine that is used to perform various operations.
     * While it is public, it is not meant to be used directly by the
     * application, and should only be used by the CDRomDevice class.
     *
     */
    template <Concepts::IsCDRomDeviceStateEnum S>
    class Action : public ActionBase {
      protected:
        Action(const char *const name) : ActionBase(name) {}
        void registerMe(CDRomDevice *device) {
            device->switchAction(this);
            m_device = device;
        }

        void setState(S state) { m_device->m_state = static_cast<uint8_t>(state); }
        S getState() const { return static_cast<S>(m_device->m_state); }
    };

  private:
    void switchAction(ActionBase *action);
    void irq();
    void actionComplete();

    friend class ActionBase;

    eastl::function<void(bool)> m_callback;
    uint32_t m_event = 0;
    ActionBase *m_action = nullptr;
    uint8_t m_state = 0;
    bool m_success = false;
    bool m_blocking = false;

    struct BlockingAction {
        BlockingAction(CDRomDevice *, GPU &);
        ~BlockingAction();

      private:
        CDRomDevice *m_device;
        GPU &m_gpu;
    };
};

}  // namespace psyqo
