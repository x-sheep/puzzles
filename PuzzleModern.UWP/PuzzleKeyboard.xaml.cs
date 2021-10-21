using PuzzleCommon;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Devices.Input;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.System;
using Windows.UI;
using Windows.UI.ViewManagement;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using Windows.UI.Xaml.Shapes;

// The User Control item template is documented at https://go.microsoft.com/fwlink/?LinkId=234236

namespace PuzzleModern.UWP
{
    public sealed partial class PuzzleKeyboard : UserControl
    {
        public static readonly DependencyProperty TwoRowsProperty = DependencyProperty.Register(
            nameof(TwoRows),
            typeof(bool),
            typeof(PuzzleKeyboard), null);

        public bool TwoRows
        {
            get => (bool)GetValue(TwoRowsProperty);
            set => SetValue(TwoRowsProperty, value);
        }

        public event EventHandler<EventArgs> ButtonBarChanged;
        public event EventHandler<ButtonBarPressedEventArgs> ButtonPressed;

        readonly SolidColorBrush _background = new SolidColorBrush(Color.FromArgb(0xFF, 0xCC, 0xCC, 0xCC));
        readonly SolidColorBrush _hovering = new SolidColorBrush(Colors.White);
        readonly SolidColorBrush _selected = new SolidColorBrush(Colors.Blue);
        readonly SolidColorBrush _text = new SolidColorBrush(Colors.Black);
        readonly SolidColorBrush _selectedText = new SolidColorBrush(Colors.White);
        private readonly UISettings uiSettings;

        bool _holding = false;
        int _total;
        VirtualButton _heldButton = null;

        VirtualButtonCollection _buttons = null;
        public VirtualButtonCollection Buttons
        {
            get => _buttons;
            set
            {
                _buttons = value;
                ButtonBarChanged?.Invoke(this, EventArgs.Empty);
                UpdateButtons();
            }
        }

        public PuzzleKeyboard()
        {
            this.InitializeComponent();
            RegisterPropertyChangedCallback(TwoRowsProperty, OnPropertyChanged);

            uiSettings = new UISettings();
            uiSettings.ColorValuesChanged += (sender, args) =>
            {
                _ = Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, ApplyColors);
            };
            ApplyColors();
        }

        private void ApplyColors()
        {
            var accent = (Color)Application.Current.Resources["SystemAccentColor"];
            _selected.Color = accent;
            _selectedText.Color = accent.IsDark() ? Colors.White : Colors.Black;
            HighlightedBorder.Background = _selected;
            HighlightedText.Foreground = _selectedText;
        }

        private void OnPropertyChanged(DependencyObject sender, DependencyProperty dp)
        {
            UpdateButtons();
        }

        private void UpdateButtons()
        {
            int x, y;

            MainGrid.Children.Clear();
            MainGrid.ColumnDefinitions.Clear();
            MainGrid.RowDefinitions.Clear();

            if (_buttons == null)
                return;

            _total = _buttons.Buttons.Count;
            var twoRows = TwoRows;

            for (var i = 0; i < (twoRows ? (_total + 1) / 2 : _total); i++)
            {
                MainGrid.ColumnDefinitions.Add(new ColumnDefinition());
            }
            for (var i = 0; i < (twoRows ? 2 : 1); i++)
            {
                MainGrid.RowDefinitions.Add(new RowDefinition());
            }

            x = y = 0;
            foreach (var b in _buttons.Buttons)
            {
                var i = (int)b.Key;

                var rect = new Rectangle
                {
                    Name = "Rect" + i,
                    Fill = _background,
                    StrokeThickness = 0,
                    Tag = b,
                    IsHitTestVisible = true
                };
                MainGrid.Children.Add(rect);

                var text = new TextBlock
                {
                    Name = "Text" + i,
                    Text = b.Name,
                    HorizontalAlignment = HorizontalAlignment.Center,
                    VerticalAlignment = VerticalAlignment.Center,
                    FontSize = 20,
                    IsHitTestVisible = false,
                    Foreground = _text
                };

                MainGrid.Children.Add(text);
                Grid.SetColumn(rect, x);
                Grid.SetColumn(text, x);

                /* Make the backspace button twice as tall, if we have an odd number of buttons */
                if (twoRows && y == 1 && x >= _total / 2 &&
                    _buttons.Buttons.Last().Key == VirtualKey.Back)
                {
                    Grid.SetRow(rect, 0);
                    Grid.SetRowSpan(rect, 2);
                    Grid.SetRow(text, 0);
                    Grid.SetRowSpan(text, 2);
                }
                else
                {
                    Grid.SetRow(rect, y);
                    Grid.SetRow(text, y);
                }

                x++;
                if (twoRows && y == 0 && x >= _total / 2 && 
                    (_buttons.Buttons.Last().Key == VirtualKey.Back || x >= (_total + 1) / 2))
                {
                    x = 0;
                    y++;
                }
            }
        }

        private void MainGrid_PointerPressed(object sender, PointerRoutedEventArgs e)
        {
            _holding = true;
            _heldButton = null;
            MainGrid.CapturePointer(e.Pointer);
            MainGrid_PointerMoved(sender, e);
        }

        private void MainGrid_PointerMoved(object sender, PointerRoutedEventArgs e)
        {
            var pointer = e.GetCurrentPoint(MainGrid).Position;

            var hitTest = MainGrid.Children.FirstOrDefault(r =>
            {
                var t = MainGrid.TransformToVisual(r);
                var relPoint = t.TransformPoint(pointer);

                return relPoint.X >= 0 && relPoint.Y >= 0 && relPoint.X <= r.ActualSize.X && relPoint.Y <= r.ActualSize.Y;
            });

            var rect = hitTest as Rectangle;
            var button = rect?.Tag as VirtualButton;

            if (_heldButton == button)
                return;

            if (_heldButton != null)
            {
                var i = (int)_heldButton.Key;
                var oldRect = (Rectangle)MainGrid.FindName("Rect" + i);
                oldRect.Fill = _background;
                var oldText = (TextBlock)MainGrid.FindName("Text" + i);
                oldText.Foreground = _text;
            }

            _heldButton = button;

            if (_heldButton == null) return;

            if (!_holding)
            {
                rect.Fill = _hovering;
            }
            else
            {
                rect.Fill = _selected;
                var text = (TextBlock)MainGrid.FindName("Text" + (int)button.Key);
                text.Foreground = _selectedText;

                if (e.Pointer.PointerDeviceType != PointerDeviceType.Mouse)
                {
                    HighlightedText.Text = button.Name;
                    HighlightedBorder.Visibility = Visibility.Visible;
                    HighlightedBorder.MinWidth = rect.ActualWidth + 12;
                    HighlightedBorder.Height = 48;
                    HighlightedBorder.UpdateLayout();
                    HighlightedText.UpdateLayout();

                    var point = MainGrid.TransformToVisual(rect).TransformPoint(default);
                    var centerX = rect.ActualWidth - HighlightedBorder.ActualWidth;
                    var x = -point.X + (centerX / 2);
                    x = Math.Min(x, MainGrid.ActualWidth);
                    var y = -point.Y;
                    HighlightedBorder.Margin = new Thickness(x, -50 + y, 0, 0);
                }
            }
        }

        private void MainGrid_PointerReleased(object sender, PointerRoutedEventArgs e)
        {
            MainGrid.ReleasePointerCapture(e.Pointer);
            if (_heldButton != null)
            {
                var i = (int)_heldButton.Key;
                var oldRect = (Rectangle)MainGrid.FindName("Rect" + i);
                oldRect.Fill = e.Pointer.PointerDeviceType == PointerDeviceType.Mouse ? _hovering : _background;
                var oldText = (TextBlock)MainGrid.FindName("Text" + i);
                oldText.Foreground = _text;
            }

            HighlightedBorder.Visibility = Visibility.Collapsed;
            if (!_holding)
                return;
            _holding = false;

            if (_heldButton != null)
            {
                ButtonPressed?.Invoke(this, new ButtonBarPressedEventArgs { Button = _heldButton });
                _heldButton = null;
            }
        }

        private void MainGrid_PointerExited(object sender, PointerRoutedEventArgs e)
        {
            HighlightedBorder.Visibility = Visibility.Collapsed;
            if (_heldButton != null)
            {
                var i = (int)_heldButton.Key;
                var oldRect = (Rectangle)MainGrid.FindName("Rect" + i);
                oldRect.Fill = _background;
                var oldText = (TextBlock)MainGrid.FindName("Text" + i);
                oldText.Foreground = _text;
                _heldButton = null;
            }
        }
    }

    public class ButtonBarPressedEventArgs : EventArgs
    {
        public VirtualButton Button { get; set; }
    }
}
