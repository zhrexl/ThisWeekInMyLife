# Schedule

![app-banner](https://user-images.githubusercontent.com/91388039/233837273-3a942cea-cdc4-48df-b595-cd2c6584d003.png)

A modern kanban-style planner designed to help you organize your week efficiently. Also known as *ThisWeekInMyLife* on Flathub, Schedule provides an intuitive interface for managing tasks and notes across customizable columns.

<a href='https://flathub.org/apps/io.github.zhrexl.thisweekinmylife'><img width='240' alt='Download on Flathub' src='https://dl.flathub.org/assets/badges/flathub-badge-en.png'/></a>

## Features

- **Drag & Drop Interface**: Seamlessly move tasks between columns with intuitive drag-and-drop functionality
- **Customizable Columns**: Add, reorder, and rename columns to fit your workflow (not limited to weekdays!)
- **Task Cards**: Create detailed task cards with expandable/collapsible content
- **Flexible Organization**: Hide or show card details as needed to maintain focus
- **Modern UI**: Built with GTK4 and libadwaita for a native, polished experience

## Installation

### Prerequisites
Ensure you have the following dependencies installed:

**Required:**
- CMake
- Meson and Ninja build system
- GNU gettext (`gettext` package)
- GTK4 development libraries (`libgtk-4-dev`)
- JSON-GLib development libraries (`libjson-glib-dev`)
- libadwaita development libraries (`libadwaita-1-dev`)

**Optional:**
- appstream-util (for metadata validation)

### Build Instructions

1. **Clone the repository:**
   ```bash
   git clone https://github.com/zhrexl/ThisWeekInMyLife.git
   ```

2. **Navigate to the project directory:**
   ```bash
   cd schedule
   ```

3. **Configure the build:**
   ```bash
   meson setup build
   ```
   
   *Optional: Specify a custom installation prefix:*
   ```bash
   meson setup build -Dprefix=/your/custom/path
   ```

4. **Build the project:**
   ```bash
   cd build
   ninja
   ```

5. **Install:**
   ```bash
   ninja install
   ```

## Development Status

This project is in active early development. We welcome contributions of all kinds, including:

- Bug reports and feature requests
- Code contributions
- UI/UX improvements
- Documentation enhancements
- Testing and feedback

## Contributing

We'd love your help in making Schedule better! Here's how you can contribute:

1. **Fork** the repository
2. **Create** a feature branch for your changes
3. **Make** your improvements
4. **Test** your changes thoroughly
5. **Submit** a pull request with a clear description of your changes

All contributions will be reviewed promptly and gratefully accepted.

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](https://github.com/zhrexl/ThisWeekInMyLife/blob/main/LICENSE) file for details.

## Support

[Add support/contact information here]
