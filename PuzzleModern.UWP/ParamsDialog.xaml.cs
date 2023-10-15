using PuzzleCommon;
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

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace PuzzleModern.UWP
{
    public static class ConfigItemExtensions
    {
        public static void RenderIntoPanel(this ConfigItem[] config, Panel panel, DataTemplate template, bool useSwitches)
        {
            for (var i = 0; i < config.Length; i++)
            {
                var item = config[i];
                switch (item.Field)
                {
                    case ConfigField.BOOLEAN:
                        if (useSwitches)
                        {
                            var toggle = new ToggleSwitch
                            {
                                Name = "ConfigItem" + i,
                                IsOn = item.IntValue != 0,
                                Header = item.Label,
                                Width = panel.Width,
                                HeaderTemplate = template,
                                Margin = new Thickness(0, 0, 0, 10)
                            };

                            panel.Children.Add(toggle);
                        }
                        else
                        {
                            var toggle = new CheckBox
                            {
                                Name = "ConfigItem" + i,
                                IsChecked = item.IntValue != 0,
                                Content = item.Label,
                                Width = panel.Width,
                                ContentTemplate = template,
                                Margin = new Thickness(0, 0, 0, 10)
                            };

                            panel.Children.Add(toggle);
                        }
                        break;
                    case ConfigField.ENUM:
                        var box = new ComboBox
                        {
                            Name = "ConfigItem" + i,
                            HeaderTemplate = template,
                            Header = item.Label,
                            Width = panel.Width,
                            Margin = new Thickness(0, 0, 0, 10)
                        };

                        foreach (var s in item.StringValues)
                        {
                            box.Items.Add(new ComboBoxItem
                            {
                                Content = s
                            });
                        }
                        box.SelectedIndex = item.IntValue;

                        panel.Children.Add(box);
                        break;
                    case ConfigField.TEXT:
                    case ConfigField.INTEGER:
                        var txt = new TextBox
                        {
                            Name = "ConfigItem" + i,
                            HeaderTemplate = template,
                            Header = item.Label,
                            Text = item.StringValue,
                            Margin = new Thickness(0, 0, 0, 10)
                        };

                        // Use the numeric keyboard.
                        if (item.Field == ConfigField.INTEGER)
                        {
                            var scope = new InputScope();
                            scope.Names.Add(new InputScopeName(InputScopeNameValue.Number));
                            txt.InputScope = scope;
                        }

                        panel.Children.Add(txt);
                        break;
                    case ConfigField.FLOAT:
                        var slider = new Slider
                        {
                            Name = "ConfigItem" + i,
                            Minimum = 0,
                            Maximum = 1,
                            StepFrequency = 0.05,
                            TickFrequency = 0.25,
                            TickPlacement = TickPlacement.BottomRight,
                            Header = item.Label,
                            HeaderTemplate = template
                        };
                        if (double.TryParse(item.StringValue, out var d))
                            slider.Value = d;

                        panel.Children.Add(slider);
                        break;
                }
            }
        }
    }

    public sealed partial class ParamsDialog
    {
        private readonly ConfigItem[] configItems = null;

        public ParamsDialog(string name, ConfigItem[] config)
        {
            configItems = config;
            this.InitializeComponent();
            NameLabel.Text = name + " parameters";

            if (config != null && config.Length > 0)
            {
                config.RenderIntoPanel(ItemsPanel, LabelTemplate, false);
            }
            else
            {
                NoConfigLabel.Visibility = Visibility.Visible;
            }
        }

        public event EventHandler<NewConfigurationEventArgs> NewConfiguration;

        private void ConfirmButton_Click(ContentDialog sender, ContentDialogButtonClickEventArgs args)
        {
            if (configItems == null) return;

            args.Cancel = true;

            for (var i = 0; i < configItems.Length; i++)
            {
                var item = configItems[i];
                var itemObj = FindName("ConfigItem" + i);
                switch (item.Field)
                {
                    case ConfigField.BOOLEAN:
                        item.IntValue = ((CheckBox)itemObj).IsChecked == true ? 1 : 0;
                        break;
                    case ConfigField.ENUM:
                        item.IntValue = ((ComboBox)itemObj).SelectedIndex;
                        break;
                    case ConfigField.TEXT:
                    case ConfigField.INTEGER:
                        item.StringValue = ((TextBox)itemObj).Text;
                        break;
                    case ConfigField.FLOAT:
                        item.StringValue = ((Slider)itemObj).Value.ToString();
                        break;
                }
            }

            var newArgs = new NewConfigurationEventArgs
            {
                NewConfig = configItems
            };
            NewConfiguration?.Invoke(this, newArgs);

            if (newArgs.Error != null)
            {
                ErrorLabel.Visibility = Visibility.Visible;
                ErrorLabel.Text = newArgs.Error;
                ErrorAppearingStoryboard.Begin();
            }
        }
    }

    public class NewConfigurationEventArgs
    {
        private string _error;
        public ConfigItem[] NewConfig { get; set; }

        public string Error
        {
            get => _error;
            set
            {
                if (value != null)
                    _error = value;
            }
        }
    }
}
