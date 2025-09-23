using Avalonia.Media;
using Avalonia.Media.Imaging;
using Avalonia.Platform;
using ReactiveUI;

namespace FulmenUI;

public class MainWindowViewModel : ReactiveObject
{
  private bool _isDarkMode = false;

  private IBrush _background = Brushes.White;
  public IBrush Background
  {
    get => _background;
    set => this.RaiseAndSetIfChanged(ref _background, value);
  }

  private IBrush _foreground = Brushes.Black;
  public IBrush Foreground
  {
    get => _foreground;
    set => this.RaiseAndSetIfChanged(ref _foreground, value);
  }

  private IImage _themeIcon;
  public IImage ThemeIcon
  {
    get => _themeIcon;
    set => this.RaiseAndSetIfChanged(ref _themeIcon, value);
  }

  private IImage _commandIcon;
  public IImage CommandIcon
  {
    get => _commandIcon;
    set => this.RaiseAndSetIfChanged(ref _commandIcon, value);
  }

  public MainWindowViewModel()
  {
    _themeIcon = LoadIcon("Assets/dark-mode.png");
    _commandIcon = LoadIcon("Assets/dark-command.png");
  }

  public void ToggleTheme()
  {
    _isDarkMode = !_isDarkMode;

    if (_isDarkMode)
    {
      Background = new SolidColorBrush(Color.Parse("#1E1E1E"));
      Foreground = Brushes.White;
      ThemeIcon = LoadIcon("Assets/light-mode.png");
      CommandIcon = LoadIcon("Assets/light-command.png");
    }
    else
    {
      Background = Brushes.White;
      Foreground = Brushes.Black;
      ThemeIcon = LoadIcon("Assets/dark-mode.png");
      CommandIcon = LoadIcon("Assets/dark-command.png");
    }
  }

  private IImage LoadIcon(string assetUri)
  {
    return new Bitmap(AssetLoader.Open(new Uri($"avares://ui/{assetUri}")));
  }
}
