# Schedule

 ![app-banner](https://user-images.githubusercontent.com/91388039/233837273-3a942cea-cdc4-48df-b595-cd2c6584d003.png)
 
Also as known as *ThisWeekInMyLife* in Flathub, a kanban-styled planner that aims to help you organize your week! The user can insert cards with tasks and notes. Drag and drop cards between different columns that can be added, switch orders and hide or expand contents of cards. You can even edit column names so they're not just about weekdays!  

This project is currently under heavy and early development, so any contribution is highly welcome.

## Install

1. git clone this repository
2. unzip & cd into folder
3. run ```meson build```
4. ```cd build```
5. ```ninja install```

## Build requirements

This project requires or depends on:

- CMake
- Meson and Ninja
- GNU gettext (available as a package! `gettext`)
- GTK4 (`libgtk-4-dev`)
- json-glib (`libjson-glib-dev`)
- libadwaita-1 (`libadwaita-1-dev`)
- appstream-util (optional)

When building, you might also want to specify custom install directory for Meson with `-Dprefix=`.

## How to contribute

Fork the repository, make changes to the repository, then submit pull request and we'll gladly review :)
