using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using System;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;
using Avalonia.Threading;
using Avalonia.Platform.Storage;
using Avalonia.Input;

namespace FulmenUI
{
  public partial class MainWindow : Window
  {
    private CancellationTokenSource? _cts;

    public MainWindow()
    {
      InitializeComponent();
      DataContext = new MainWindowViewModel();

      BrowseButton.Click += OnBrowseClicked;
      RunButton.Click += OnRunClicked;
      CancelButton.Click += OnCancelClicked;
    }

    private async void OnBrowseClicked(object? sender, RoutedEventArgs e)
    {
      var result = await StorageProvider.OpenFilePickerAsync(new FilePickerOpenOptions
      {
        AllowMultiple = false,
        Title = "Choose image tarball or binary"
      });

      if (result.Count > 0)
      {
        ImagePathBox.Text = result[0].Path.LocalPath;
      }
    }

    private async void OnRunClicked(object? sender, RoutedEventArgs e)
    {
      if (_cts != null)
        return;

      var image = ImagePathBox.Text ?? string.Empty;
      var cmd = CommandBox.Text ?? string.Empty;

      Progress.IsVisible = true;
      StatusLabel.Text = "Running...";
      OutputBox.Text = string.Empty;

      _cts = new CancellationTokenSource();
      try
      {
        await RunProcessAsync("/bin/echo", new[] { $"Running: {image} {cmd}" }, _cts.Token);
        StatusLabel.Text = "Finished";
        CommandBox.Text = string.Empty;
      }
      catch (OperationCanceledException)
      {
        StatusLabel.Text = "Cancelled";
      }
      finally
      {
        Progress.IsVisible = false;
        _cts = null;
      }
    }

    private void OnCancelClicked(object? sender, RoutedEventArgs e)
    {
      _cts?.Cancel();
    }

    private void OnToggleThemeClicked(object? sender, RoutedEventArgs e)
    {
      if (DataContext is MainWindowViewModel viewModel)
      {
        viewModel.ToggleTheme();
      }
    }

    private Task RunProcessAsync(string fileName, string[] args, CancellationToken ct)
    {
      return Task.Run(() =>
      {
        var psi = new ProcessStartInfo(fileName)
        {
          RedirectStandardOutput = true,
          RedirectStandardError = true,
          UseShellExecute = false,
        };

        foreach (var a in args)
          psi.ArgumentList.Add(a);

        using var p = Process.Start(psi)!;

        void readStream(System.IO.StreamReader r)
        {
          while (!r.EndOfStream)
          {
            var line = r.ReadLine();
            if (line != null)
            {
              Dispatcher.UIThread.Post(() =>
                    {
                      OutputBox.Text += line + "\n";
                      OutputBox.CaretIndex = OutputBox.Text?.Length ?? 0;
                    });
            }
            if (ct.IsCancellationRequested)
            {
              try { p.Kill(); } catch { }
              ct.ThrowIfCancellationRequested();
            }
          }
        }

        var outTask = Task.Run(() => readStream(p.StandardOutput));
        var errTask = Task.Run(() => readStream(p.StandardError));
        p.WaitForExit();
        Task.WaitAll(outTask, errTask);
      }, ct);
    }
  }
}
