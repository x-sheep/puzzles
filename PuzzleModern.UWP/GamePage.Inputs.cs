using PuzzleCommon;
using System;
using System.Linq;
using System.Collections.Generic;
using System.Threading.Tasks;
using Windows.ApplicationModel.DataTransfer;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Graphics.Display;
using Windows.Storage;
using Windows.System;
using Windows.System.Threading;
using Windows.UI.Core;
using Windows.UI.Popups;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Navigation;
using Windows.UI.Xaml.Controls.Primitives;

namespace PuzzleModern.UWP
{
    partial class GamePage
    {
        private ThreadPoolTimer RightClickTimer;
        ButtonType _leftAction, _middleAction, _rightAction, _touchAction, _holdAction;
        bool _leftPressed, _middlePressed, _rightPressed, _initialPressed, _touchPressed, _holdPressed;
        bool _undoHotkey, _redoHotkey;
        bool _ctrlPressed, _shiftPressed;
        Point _initialPoint;

        private void DrawCanvas_RightTapped(object sender, RightTappedRoutedEventArgs e)
        {
            if (_generatingGame)
                return;

            /* 
             * This is used to suppress the appbar appearing when right-clicking inside the game canvas.
             * Actual right taps are processed by the PointerPressed and PointerReleased events.
             */
            e.Handled = true;
        }

        private void DrawCanvas_PointerPressed(object sender, PointerRoutedEventArgs e)
        {
            DrawCanvas.Focus(FocusState.Pointer);

            if (_generatingGame)
                return;

            var ptrPt = e.GetCurrentPoint(DrawCanvas);
            int x = (int)ptrPt.Position.X, y = (int)ptrPt.Position.Y;
            /* 
             * When using touch, we use gestures to differentiate between left and right clicks.
             * Tapping and dragging count as left button, while holding counts as right button.
             * If this is a right button, we can not yet process the Pressed event.
             */
            if (e.Pointer.PointerDeviceType != Windows.Devices.Input.PointerDeviceType.Touch || fe.IsRightButtonDisabled())
            {
                if (ptrPt.Properties.IsLeftButtonPressed)
                {
                    fe.SendClick(x, y, _leftAction, ButtonState.DOWN);
                    _leftPressed = true;
                    e.Handled = true;
                }
                if (ptrPt.Properties.IsMiddleButtonPressed)
                {
                    fe.SendClick(x, y, _middleAction, ButtonState.DOWN);
                    _middlePressed = true;
                    e.Handled = true;
                }
                if (ptrPt.Properties.IsRightButtonPressed)
                {
                    fe.SendClick(x, y, _rightAction, ButtonState.DOWN);
                    _rightPressed = true;
                    e.Handled = true;
                }
            }

            /*
            * When using touch, we use gestures to differentiate between left and right clicks.
            * Tapping and dragging count as left button, while holding counts as right button.
            * If this is a right button, we can not yet process the Pressed event.
            */
            else
            {
                _initialPoint = ptrPt.Position;
                _initialPressed = true;
                e.Handled = true;
                var period = TimeSpan.FromSeconds(0.6);
                RightClickTimer?.Cancel();
                RightClickTimer = ThreadPoolTimer.CreateTimer(timer =>
                {
                    _ = Dispatcher.RunAsync(CoreDispatcherPriority.High, () =>
                    {
                        if (!_generatingGame && _initialPressed)
                        {
                            int ix = (int)_initialPoint.X, iy = (int)_initialPoint.Y;
                            _initialPressed = false;
                            fe.SendClick(ix, iy, _holdAction, ButtonState.DOWN);
                            _holdPressed = true;
                            DrawCanvas.Pulsate(ix, iy);
                        }
                    });
                }, period);
            }
        }

        private void pageRoot_PointerPressed(object sender, PointerRoutedEventArgs e)
        {
            if (_generatingGame)
                return;

            var ptrPt = e.GetCurrentPoint(null);
            if (ptrPt.Properties.IsXButton1Pressed)
            {
                e.Handled = true;
                fe.Undo();
            }
            else if (ptrPt.Properties.IsXButton2Pressed)
            {
                e.Handled = true;
                fe.Redo();
            }
        }

        const int LEFT_DISTANCE = 10;

        private void pageRoot_PointerMoved(object sender, PointerRoutedEventArgs e)
        {
            if (_generatingGame)
                return;

            var ptrPt = e.GetCurrentPoint(DrawCanvas);
            double dx = ptrPt.Position.X, dy = ptrPt.Position.Y;

            /* When moving outside a certain region, confirm this as a left drag */
            if (_initialPressed)
            {
                double xdiff = dx - _initialPoint.X, ydiff = dy - _initialPoint.Y;
                if ((xdiff * xdiff) + (ydiff * ydiff) > LEFT_DISTANCE * LEFT_DISTANCE)
                {
                    _initialPressed = false;
                    fe.SendClick((int)_initialPoint.X, (int)_initialPoint.Y, _touchAction, ButtonState.DOWN);
                    _touchPressed = true;
                }
            }

            int x = (int)dx, y = (int)dy;
            if (_leftPressed)
            {
                fe.SendClick(x, y, _leftAction, ButtonState.DRAG);
                e.Handled = true;
            }
            if (_middlePressed)
            {
                fe.SendClick(x, y, _middleAction, ButtonState.DRAG);
                e.Handled = true;
            }
            if (_rightPressed)
            {
                fe.SendClick(x, y, _rightAction, ButtonState.DRAG);
                e.Handled = true;
            }
            if (_touchPressed)
            {
                fe.SendClick(x, y, _touchAction, ButtonState.DRAG);
                e.Handled = true;
            }
            if (_holdPressed)
            {
                fe.SendClick(x, y, _holdAction, ButtonState.DRAG);
                e.Handled = true;
            }
        }

        private void pageRoot_PointerReleased(object sender, PointerRoutedEventArgs e)
        {
            if (_generatingGame)
                return;

            var ptrPt = e.GetCurrentPoint(DrawCanvas);
            int x = (int)ptrPt.Position.X, y = (int)ptrPt.Position.Y;

            if (_leftPressed)
            {
                fe.SendClick(x, y, _leftAction, ButtonState.UP);
                e.Handled = true;
            }
            if (_middlePressed)
            {
                fe.SendClick(x, y, _middleAction, ButtonState.UP);
                e.Handled = true;
            }
            if (_rightPressed)
            {
                fe.SendClick(x, y, _rightAction, ButtonState.UP);
                e.Handled = true;
            }
            if (_touchPressed)
            {
                fe.SendClick(x, y, _touchAction, ButtonState.UP);
                e.Handled = true;
            }
            if (_holdPressed)
            {
                fe.SendClick(x, y, _holdAction, ButtonState.UP);
                e.Handled = true;
            }
            if (_initialPressed)
            {
                fe.SendClick((int)_initialPoint.X, (int)_initialPoint.Y, _touchAction, ButtonState.DOWN);
                fe.SendClick(x, y, _touchAction, ButtonState.UP);
                e.Handled = true;
            }

            _leftPressed = _middlePressed = _rightPressed = false;
            _touchPressed = _holdPressed = _initialPressed = false;

            RightClickTimer?.Cancel();
            RightClickTimer = null;
            UpdateUndoButtons();
        }

        private bool ShouldIgnoreKeys()
        {
            return _isFlyoutOpen || GameMenu.Flyout.IsOpen || PresetMenu.Flyout.IsOpen || HelpMenu.Flyout.IsOpen;
        }

        private void MenuFlyout_Opening(object sender, object e)
        {
            ((MenuFlyout)sender).OverlayInputPassThroughElement = CommandBar;
            var menu = new[] { GameMenu, PresetMenu, HelpMenu }.First(m => m.Flyout == sender);
            menu.IsEnabled = false;
        }

        private void MenuFlyout_Closing(FlyoutBase sender, FlyoutBaseClosingEventArgs args)
        {
            var menu = new[] { GameMenu, PresetMenu, HelpMenu }.First(m => m.Flyout == sender);
            menu.IsEnabled = true;
        }

        private void OnActivated(CoreWindow sender, WindowActivatedEventArgs args)
        {
            GameMenu.IsEnabled = true;
            PresetMenu.IsEnabled = true;
            HelpMenu.IsEnabled = true;
        }

        private void AppBarButton_PointerEntered(object sender, PointerRoutedEventArgs e)
        {
            var menu = (AppBarButton)sender;

            if (!menu.Flyout.IsOpen && (GameMenu.Flyout.IsOpen || PresetMenu.Flyout.IsOpen || HelpMenu.Flyout.IsOpen))
                menu.Flyout.ShowAt(menu);
        }

        private void OnAcceleratorKeyActivated(CoreDispatcher sender, AcceleratorKeyEventArgs e)
        {
            if (e.KeyStatus.IsMenuKeyDown) return;

            var k = e.VirtualKey;
            var numpad = !e.KeyStatus.IsExtendedKey;

            if (k == VirtualKey.Shift)
                _shiftPressed = e.EventType == CoreAcceleratorKeyEventType.KeyDown;
            if (k == VirtualKey.Control)
                _ctrlPressed = e.EventType == CoreAcceleratorKeyEventType.KeyDown;
            if (k == VirtualKey.F1 && e.EventType == CoreAcceleratorKeyEventType.KeyDown)
            {
                if (ShouldIgnoreKeys())
                    return;

                ButtonHelp_Click(sender, null);
                e.Handled = true;
            }

            var focus = FocusManager.GetFocusedElement() as Control;
            if (focus != null && focus.FocusState == FocusState.Keyboard && !ReferenceEquals(focus, DrawCanvas))
            {
                var menus = new List<AppBarButton> { GameMenu, PresetMenu, HelpMenu };
                var openMenu = menus.FindIndex(m => m.Flyout.IsOpen);

                if (openMenu != -1 && k == VirtualKey.Left && e.EventType == CoreAcceleratorKeyEventType.KeyDown)
                {
                    var newMenu = ((openMenu - 1) + menus.Count) % menus.Count;
                    menus[newMenu].Flyout.ShowAt(menus[newMenu]);
                    e.Handled = true;
                }
                else if (openMenu != -1 && k == VirtualKey.Right && e.EventType == CoreAcceleratorKeyEventType.KeyDown)
                {
                    var newMenu = (openMenu + 1) % menus.Count;
                    menus[newMenu].Flyout.ShowAt(menus[newMenu]);
                    e.Handled = true;
                }
                else if ((k == VirtualKey.Escape && e.EventType == CoreAcceleratorKeyEventType.KeyDown)
                    || (k == VirtualKey.Menu && e.EventType == CoreAcceleratorKeyEventType.SystemKeyUp))
                {
                    if (openMenu != -1)
                        menus[openMenu].Flyout.Hide();
                    DrawCanvas.Focus(FocusState.Keyboard);
                    e.Handled = true;
                }
                else if (!ShouldIgnoreKeys() && k == VirtualKey.Down && focus?.FocusState == FocusState.Keyboard)
                {
                    if (focus is AppBarButton menu && menu.Flyout != null)
                        menu.Flyout.ShowAt(menu);
                    e.Handled = true;
                }

                return;
            }

            if (k == VirtualKey.Menu && e.EventType == CoreAcceleratorKeyEventType.SystemKeyUp)
            {
                GameMenu.Focus(FocusState.Keyboard);
                e.Handled = true;
                return;
            }

            if (k == VirtualKey.Escape && e.EventType == CoreAcceleratorKeyEventType.KeyDown)
            {
                if (ShouldIgnoreKeys())
                    return;

                Frame.GoBack();
                e.Handled = true;
            }

            if (_generatingGame || ShouldIgnoreKeys())
                return;

            // Simulate NumLock always being enabled
            if (k == VirtualKey.Home)
                k = VirtualKey.NumberPad7;
            else if (k == VirtualKey.Up && numpad)
                k = VirtualKey.NumberPad8;
            else if (k == VirtualKey.PageUp)
                k = VirtualKey.NumberPad9;
            else if (k == VirtualKey.Left && numpad)
                k = VirtualKey.NumberPad4;
            else if (k == VirtualKey.Clear && numpad)
                k = VirtualKey.NumberPad5;
            else if (k == VirtualKey.Right && numpad)
                k = VirtualKey.NumberPad6;
            else if (k == VirtualKey.End)
                k = VirtualKey.NumberPad1;
            else if (k == VirtualKey.Down && numpad)
                k = VirtualKey.NumberPad2;
            else if (k == VirtualKey.PageDown)
                k = VirtualKey.NumberPad3;
            else if (k == VirtualKey.Insert && numpad)
                k = VirtualKey.NumberPad0;
            else if (k == VirtualKey.Delete)
                k = numpad ? VirtualKey.Decimal : VirtualKey.Back;

            if (e.EventType == CoreAcceleratorKeyEventType.KeyDown)
            {
                if ((_ctrlPressed && !_shiftPressed && k == VirtualKey.Z) || (_undoHotkey && k == VirtualKey.U))
                {
                    fe.Undo();
                    e.Handled = true;
                    UpdateUndoButtons();
                }
                if ((_ctrlPressed && k == VirtualKey.Y) || (_redoHotkey && k == VirtualKey.R)
                    || (_ctrlPressed && _shiftPressed && k == VirtualKey.Z))
                {
                    fe.Redo();
                    e.Handled = true;
                    UpdateUndoButtons();
                }
                if (_ctrlPressed && k == VirtualKey.N)
                {
                    BeginNewGame();
                    e.Handled = true;
                }
            }
            if (!e.Handled && ((k >= VirtualKey.Number0 && k <= VirtualKey.Number9) /* Number keys */
                || (k >= VirtualKey.NumberPad0 && k <= VirtualKey.NumberPad9) /* Number pad */
                || k == VirtualKey.Enter || k == VirtualKey.Space /* Button one and two */
                || (k >= VirtualKey.Left && k <= VirtualKey.Down) /* Directional keys */
                || (k >= VirtualKey.A && k <= VirtualKey.Z) /* Letters */
                || k == VirtualKey.Delete || k == VirtualKey.Back /* Delete */
                || k == VirtualKey.Escape
                ))
            {
                /* Only send key down events. */
                if (e.EventType == CoreAcceleratorKeyEventType.KeyDown &&
                    (_controls == null || k != _controls.ColorBlindKey))
                    fe.SendKey(k, _shiftPressed, _ctrlPressed);
                e.Handled = true;
                UpdateUndoButtons();
            }
        }
    }
}
