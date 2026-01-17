#pragma once
#include <framedot/input/Event.hpp>
#include <framedot/input/Key.hpp>

#include <array>
#include <cstddef>
#include <cstdint>


namespace framedot::input {

    /// @brief 프레임 단위 입력 상태 스냅샷
    /// - 이벤트 큐가 오버플로로 기록을 드랍해도, 상태는 최신으로 유지
    /// - 멀티스레드 업데이트를 위해 update 구간에서 읽기 전용으로 다루기 쉬운 형태 목표
    class InputState {
    public:
        /// @brief 프레임 시작 시 호출
        /// - just_pressed / just_released 같은 프레임 한정 플래그를 리셋
        void begin_frame() noexcept {
            m_just_pressed.fill(false);
            m_just_released.fill(false);
            m_any_input = false;
        }

        /// @brief 이벤트를 상태에 반영
        void apply(const Event& ev) noexcept {
            m_any_input = true;

            if (ev.type == EventType::Key) {
                apply_key_(ev.data.key);
                return;
            }

            /// TODO: 마우스 입력 등은 추후 확장
        }

        /// @brief 현재 key가 눌려져 있는지 확인
        bool key_down(Key k) const noexcept {
            const std::size_t idx = key_index_(k);
            return (idx < kKeyCount) ? m_down[idx] : false;
        }

        /// @brief 이번 프레임에 눌렸는지 확인 (edge)
        bool key_just_pressed(Key k) const noexcept {
            const std::size_t idx = key_index_(k);
            return (idx < kKeyCount) ? m_just_pressed[idx] : false;
        }

        /// @brief 이번 프레임에 떼졌는지 확인 (edge)
        bool key_just_released(Key k) const noexcept {
            const std::size_t idx = key_index_(k);
            return (idx < kKeyCount) ? m_just_released[idx] : false;
        }

        /// @brief 이번 프레임에 어떤 입력이라도 들어왔는지 확인 (가벼운 최적화용)
        bool any_input() const noexcept { return m_any_input; }

    private:
        static constexpr std::size_t kKeyCount =
            static_cast<std::size_t>(Key::Count);

        static constexpr std::size_t key_index_(Key k) noexcept {
            return static_cast<std::size_t>(k);
        }

        void apply_key_(const KeyEvent& kev) noexcept {
            const std::size_t idx = key_index_(kev.key);
            if (idx >= kKeyCount) return;

            if (kev.action == KeyAction::Press) {
                /// @brief 이미 눌려있던 키라면 just_pressed는 false 유지
                if (!m_down[idx]) {
                    m_down[idx] = true;
                    m_just_pressed[idx] = true;
                }
                return;
            }

            if (kev.action == KeyAction::Release) {
                if (m_down[idx]) {
                    m_down[idx] = false;
                    m_just_released[idx] = true;
                }
                return;
            }

            if (kev.action == KeyAction::Repeat) {
                /// @brief Repeat는 상태 변화가 아니라 "반복 입력"이므로 down 유지
                m_down[idx] = true;
                return;
            }
        }

        std::array<bool, kKeyCount> m_down{};
        std::array<bool, kKeyCount> m_just_pressed{};
        std::array<bool, kKeyCount> m_just_released{};
        bool m_any_input{false};
    };

} // namespace framedot::input