using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using PuzzleCommon;
using Windows.ApplicationModel.DataTransfer;
using Windows.Storage;
using Windows.Storage.Pickers;
using Windows.UI.Popups;
using Windows.UI.StartScreen;
using Windows.UI.Input;
using System.Threading.Tasks;
using Windows.Devices.Input;
using Windows.UI.Core;
using Windows.System;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace PuzzleModern.UWP
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        public IObservableMap<string, object> DefaultViewModel
        {
            get { return (IObservableMap<string, object>)GetValue(_DefaultViewModel); }
            set { SetValue(_DefaultViewModel, value); }
        }

        public readonly DependencyProperty _DefaultViewModel = DependencyProperty.Register(
            nameof(DefaultViewModel),
            typeof(IObservableMap<string, object>),
            typeof(MainPage), null);

        private readonly PuzzleList _puzzles;
        bool _isFlyoutOpen;

        public MainPage()
        {
            DefaultViewModel = new PropertySet();
            this.InitializeComponent();

            _puzzles = WindowsModern.GetPuzzleList();
            DefaultViewModel["Items"] = _puzzles.Items;
            DefaultViewModel["Favourites"] = _puzzles.Favourites;

            DefaultViewModel["PinVisible"] = Visibility.Collapsed;
            DefaultViewModel["UnpinVisible"] = Visibility.Collapsed;
            DefaultViewModel["FavVisible"] = Visibility.Collapsed;
            DefaultViewModel["UnfavVisible"] = Visibility.Collapsed;

            var localSettings = ApplicationData.Current.LocalSettings.Values;
            bool hideFavouritesFooter = localSettings["feature_favourites"] as bool? == true && _puzzles.Favourites.Any();
            DefaultViewModel["FavFooterVisible"] = hideFavouritesFooter ? Visibility.Collapsed : Visibility.Visible;
        }

        private void pageRoot_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            ApplyWindowSize(e.NewSize.Width, e.NewSize.Height);
        }

        const int ZOOMED_WIDTH = 960;
        const int ZOOMED_HEIGHT = 420;
        private void ApplyWindowSize(double newWidth, double newHeight)
        {
            bool pageCompact = newWidth < ZOOMED_WIDTH || newHeight < ZOOMED_HEIGHT;

            var settings = ApplicationData.Current.RoamingSettings.Values;
            bool zoomedOut = settings["feature_zoom_out"] as bool? == true;
            if (pageCompact)
            {
                DefaultViewModel["ZoomOutVisible"] = Visibility.Collapsed;
                DefaultViewModel["ZoomInVisible"] = Visibility.Collapsed;
            }
            else
            {
                DefaultViewModel["ZoomInVisible"] = zoomedOut ? Visibility.Visible : Visibility.Collapsed;
                DefaultViewModel["ZoomOutVisible"] = !zoomedOut ? Visibility.Visible : Visibility.Collapsed;
            }

            if (itemGridView != null)
            {
                if ((pageCompact || zoomedOut) && itemGridView.ItemTemplate != SmallPuzzleTemplate)
                    itemGridView.ItemTemplate = SmallPuzzleTemplate;
                else if (!pageCompact && !zoomedOut && itemGridView.ItemTemplate != NormalPuzzleTemplate)
                    itemGridView.ItemTemplate = NormalPuzzleTemplate;
            }

            if (favouritesGridView != null)
            {
                if (pageCompact && favouritesGridView.ItemTemplate != SmallFavouriteTemplate)
                    favouritesGridView.ItemTemplate = SmallFavouriteTemplate;
                else if (!pageCompact && favouritesGridView.ItemTemplate != FavouriteTemplate)
                    favouritesGridView.ItemTemplate = FavouriteTemplate;
            }

            var pad = GridRoot.Padding;
            pad.Left = newWidth % 230 / 2;
            GridRoot.Padding = pad;
        }

        private void itemGridView_Loaded(object sender, RoutedEventArgs e)
        {
            itemGridView.SelectedItem = null;
            ApplyWindowSize(pageRoot.ActualWidth, pageRoot.ActualHeight);
        }

        private void itemGridView_ItemClick(object sender, ItemClickEventArgs e)
        {
            var item = (Puzzle)e.ClickedItem;
            App.Current.ActivatePuzzle(new GameLaunchParameters(item.Name));
        }

        private async void ButtonOpen_Click(object sender, RoutedEventArgs e)
        {
            var openPicker = new FileOpenPicker
            {
                ViewMode = PickerViewMode.List,
                SuggestedStartLocation = PickerLocationId.DocumentsLibrary,
                CommitButtonText = "Load game",
            };
            openPicker.FileTypeFilter.Add(".puzzle");
            openPicker.FileTypeFilter.Add(".sav");

            var file = await openPicker.PickSingleFileAsync();
            if (file == null) return;

            await App.Current.ActivateFile(file);
        }

        private void ButtonSettings_Click(object sender, RoutedEventArgs e)
        {
            _isFlyoutOpen = true;
            var flyout = new GeneralSettingsFlyout();
            flyout.Unloaded += (s, a) => { _isFlyoutOpen = false; };
            flyout.ShowIndependent();
        }

        private void ButtonHelp_Click(object sender, RoutedEventArgs e)
        {
            _isFlyoutOpen = true;
            var flyout = new HelpFlyout();
            flyout.Unloaded += (s, a) => { _isFlyoutOpen = false; };
            flyout.ShowIndependent();
        }

        private void ButtonZoomOut_Click(object sender, RoutedEventArgs e)
        {
            ApplicationData.Current.RoamingSettings.Values["feature_zoom_out"] = true;
            itemGridView.ItemTemplate = SmallPuzzleTemplate;
            DefaultViewModel["ZoomInVisible"] = Visibility.Visible;
            DefaultViewModel["ZoomOutVisible"] = Visibility.Collapsed;
        }

        private void ButtonZoomIn_Click(object sender, RoutedEventArgs e)
        {
            ApplicationData.Current.RoamingSettings.Values["feature_zoom_out"] = false;
            itemGridView.ItemTemplate = NormalPuzzleTemplate;
            DefaultViewModel["ZoomInVisible"] = Visibility.Collapsed;
            DefaultViewModel["ZoomOutVisible"] = Visibility.Visible;
        }

        private void GridViewItem_ContextRequested(object s, ContextRequestedEventArgs e)
        {
            var element = (ContentControl)s;
            var item = (Puzzle)element.DataContext;
            var menu = new MenuFlyout();

            var settings = ApplicationData.Current.RoamingSettings.Values;
            bool isFav = _puzzles.IsFavourite(item);
            var favCommand = new MenuFlyoutItem
            {
                Text = !isFav ? "Add to favourites" : "Remove from favourites",
                Icon = new SymbolIcon(!isFav ? Symbol.OutlineStar : Symbol.SolidStar)
            };
            favCommand.Click += (sender, args) =>
            {
                ApplicationData.Current.LocalSettings.Values["feature_favourites"] = true;
                settings["fav_" + item.Name] = !isFav;
                if (isFav)
                    _puzzles.RemoveFavourite(item);
                else
                    _puzzles.AddFavourite(item);

                if (_puzzles.Favourites.Count == 0)
                    DefaultViewModel["FavFooterVisible"] = Visibility.Visible;
                else
                    DefaultViewModel["FavFooterVisible"] = Visibility.Collapsed;
            };
            menu.Items.Add(favCommand);

            bool pinExists = SecondaryTile.Exists(item.HelpName);
            var pinCommand = new MenuFlyoutItem
            {
                Text = !pinExists ? "Pin to Start" : "Unpin from Start",
                Icon = new SymbolIcon(!pinExists ? Symbol.Pin : Symbol.UnPin)
            };
            pinCommand.Click += async (sender, args) =>
            {
                var id = item.HelpName;
                if (!pinExists)
                {
                    var startArgs = item.Name;
                    var imageUri = item.ImageUri;
                    var iconUri = item.IconUri;

                    var tile = new SecondaryTile(id, item.Name, startArgs, imageUri, TileSize.Default);
                    tile.VisualElements.Square70x70Logo = iconUri;
                    tile.VisualElements.Square30x30Logo = iconUri;
                    tile.VisualElements.BackgroundColor = Windows.UI.Colors.LightGray;
                    await tile.RequestCreateAsync();
                }
                else
                {
                    var tile = new SecondaryTile(id);
                    await tile.RequestDeleteAsync();
                }
            };

            menu.Items.Add(pinCommand);
            if (e.TryGetPosition(element, out var origin))
                menu.ShowAt(element, origin);
            else
                menu.ShowAt(element, new Point(40, 40));
        }

        private void pageRoot_DragOver(object sender, DragEventArgs e)
        {
            e.AcceptedOperation = DataPackageOperation.Copy;
            e.DragUIOverride.Caption = "Load game";
            e.DragUIOverride.IsContentVisible = false;
        }

        private async void pageRoot_Drop(object sender, DragEventArgs e)
        {
            if (e.DataView.Contains(StandardDataFormats.StorageItems))
            {
                var items = await e.DataView.GetStorageItemsAsync();
                var item = items.FirstOrDefault() as StorageFile;
                if (item != null)
                    _ = App.Current.ActivateFile(item);
            }
        }

        private void ButtonAbout_Click(object sender, RoutedEventArgs e)
        {
            _isFlyoutOpen = true;
            var flyout = new AboutFlyout();
            flyout.Unloaded += (s, a) => { _isFlyoutOpen = false; };
            flyout.ShowIndependent();
        }

        private void favouritesGridView_Loaded(object sender, RoutedEventArgs e)
        {
            favouritesGridView.SelectedItem = null;
            ApplyWindowSize(pageRoot.ActualWidth, pageRoot.ActualHeight);
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            Window.Current.CoreWindow.Dispatcher.AcceleratorKeyActivated += OnAcceleratorKeyActivated;
            base.OnNavigatedTo(e);
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            base.OnNavigatedFrom(e);
            Window.Current.CoreWindow.Dispatcher.AcceleratorKeyActivated -= OnAcceleratorKeyActivated;
        }

        private void OnAcceleratorKeyActivated(CoreDispatcher sender, AcceleratorKeyEventArgs args)
        {
            if (_isFlyoutOpen) return;
            var focus = FocusManager.GetFocusedElement() as Control;

            if (args.VirtualKey == VirtualKey.F1)
            {
                ButtonHelp_Click(this, null);
                args.Handled = true;
            }

            // Press alt to toggle focus between the More button, and one of the two grids.
            // It also closes the More menu if it's open.
            if (args.VirtualKey == VirtualKey.Menu && args.EventType == CoreAcceleratorKeyEventType.SystemKeyUp)
            {
                if (MoreMenu.FocusState == FocusState.Keyboard || MoreMenu.Flyout.IsOpen)
                {
                    MoreMenu.Flyout.Hide();
                    if (lastFocused.TryGetTarget(out var item))
                        item.Focus(FocusState.Keyboard);
                    else
                        (lastFocusedParent ?? favouritesGridView).Focus(FocusState.Keyboard);
                }
                else
                {
                    MoreMenu.Focus(FocusState.Keyboard);
                }
            }

            // Pressing escape will close the More menu if it's open.
            if (args.VirtualKey == VirtualKey.Escape && args.EventType == CoreAcceleratorKeyEventType.KeyDown)
            {
                if (MoreMenu.FocusState == FocusState.Keyboard || MoreMenu.Flyout.IsOpen)
                {
                    MoreMenu.Flyout.Hide();
                    if (lastFocused.TryGetTarget(out var item))
                        item.Focus(FocusState.Keyboard);
                    else
                        (lastFocusedParent ?? favouritesGridView).Focus(FocusState.Keyboard);

                    args.Handled = true;
                }
            }

            // Press Down Arrow when the More button has focus to open its menu.
            if (args.VirtualKey == VirtualKey.Down && args.EventType == CoreAcceleratorKeyEventType.KeyDown)
            {
                if (MoreMenu.FocusState == FocusState.Keyboard && !MoreMenu.Flyout.IsOpen)
                {
                    MoreMenu.Flyout.ShowAt(MoreMenu);
                    args.Handled = true;
                }
            }

            // Moving the selection with the keyboard should treat both GridViews as if it were one list of items.
            // To do this, we manually perform the cursor movements when the cursor is in certain positions, and
            // use the default behaviour in all other cases.
            //
            // When moving up or down, the expected behaviour is to jump to another vertically aligned item.
            // When moving left or right, the cursor can jump between the end of a row and the start of the next row.

            // Only perform movements when the keyboard is focused on a grid item.
            if (args.EventType == CoreAcceleratorKeyEventType.KeyDown &&
                lastFocused.TryGetTarget(out var last) &&
                last == focus)
            {
                var centerX = focus.TransformToVisual(this).TransformPoint(new Point(focus.ActualWidth / 2, 0)).X;
                int? newIndex = null;
                GridView newParent = null;
                var limit = _puzzles.Favourites.Count - 1;

                if (lastFocusedParent == itemGridView && limit >= 0)
                {
                    newParent = favouritesGridView;
                    // Jump to the row above it, which is located on favouritesGridView.
                    if (args.VirtualKey == VirtualKey.Left && itemGridView.SelectedIndex == 0)
                        newIndex = limit;
                    if (args.VirtualKey == VirtualKey.Up)
                    {
                        // Check if the current item is located on the top row of itemGridView.
                        var distanceToTop = focus.TransformToVisual(itemGridView).TransformPoint(new Point(0, -focus.Margin.Top)).Y;
                        if (distanceToTop == 0)
                            newIndex = FirstIndexWithPosition(favouritesGridView, centerX, limit, 0, -1) ?? limit;
                    }
                }
                else if (lastFocusedParent == favouritesGridView)
                {
                    newParent = itemGridView;
                    // Jump to the row below it, which is located on itemGridView.
                    if (args.VirtualKey == VirtualKey.Right && favouritesGridView.SelectedIndex == limit)
                        newIndex = 0;
                    if (args.VirtualKey == VirtualKey.Down)
                    {
                        // A vertical jump can occur on the last row, or the penultimate row.
                        // We iterate backwards through the favourites to check for other grid items located below the current one.
                        // If there's no other grid items, the loop will find the currently focused item.
                        if (FirstIndexWithPosition(favouritesGridView, centerX, limit, 0, -1) == favouritesGridView.IndexFromContainer(focus))
                            newIndex = FirstIndexWithPosition(itemGridView, centerX, 0, _puzzles.Items.Count, 1) ?? 0;
                    }
                }

                if (newIndex.HasValue)
                {
                    var newFocus = (GridViewItem)newParent.ContainerFromIndex(newIndex.Value);
                    newFocus?.Focus(FocusState.Keyboard);
                    newParent.SelectedIndex = newIndex.Value;
                    args.Handled = true;
                }
            }
        }

        /// <summary>
        /// Find a target index to jump to, when switching focus from one GridView to the other.
        /// </summary>
        /// <param name="grid">The GridView to scan.</param>
        /// <param name="centerX">The X coordinate to find, relative to the grid's position.</param>
        /// <param name="start">The start index.</param>
        /// <param name="end">The inclusive end index.</param>
        /// <param name="d">The direction to search in. Must be <code>-1</code> or <code>+1</code>.</param>
        /// <returns>The index with the given X coordinate, or null if none was found.</returns>
        public int? FirstIndexWithPosition(GridView grid, double centerX, int start, int end, int d)
        {
            // UWP does not have a HitTest function, so we have to manually iterate through all grid items to find a hit.
            for (var idx = start; true; idx += d)
            {
                var newFocus = (FrameworkElement)grid.ContainerFromIndex(idx);
                if (newFocus == null) continue;
                var newCenterX = newFocus.TransformToVisual(this).TransformPoint(new Point(newFocus.ActualWidth / 2, 0)).X;

                if (Math.Abs(centerX - newCenterX) < (newFocus.ActualWidth + newFocus.Margin.Left + newFocus.Margin.Right) / 2)
                    return idx;
                if (idx == end)
                    return null;
            }
        }

        private void PuzzleItem_GotFocus(object sender, RoutedEventArgs e)
        {
            lastFocused.SetTarget((GridViewItem)sender);
            lastFocusedParent = itemGridView;
        }
        private void FavouriteItem_GotFocus(object sender, RoutedEventArgs e)
        {
            lastFocused.SetTarget((GridViewItem)sender);
            lastFocusedParent = favouritesGridView;
        }

        /// <summary>
        /// The grid item that was selected before the More menu was opened.
        /// </summary>
        private WeakReference<GridViewItem> lastFocused = new WeakReference<GridViewItem>(null);

        /// <summary>
        /// The parent of the item stored in <see cref="lastFocused"/>
        /// </summary>
        private GridView lastFocusedParent = null;
    }
}
