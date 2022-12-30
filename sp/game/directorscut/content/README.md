# Content folder

This is a folder that contains content subfolders, loaded in order of the folders in the `gameinfo.txt` file.

## Usage

Use the `earliest`, `early`, `late`, and `latest` folders to load content at specific times.

- `earliest`: Loaded before anything else
        - Nothing can override these
- `early`: Loaded after the game path (in this case, `directorscut`)
        - Content in the game path and custom directories can override these
- `late`: Loaded after all hl2 and mapbase content
        - The above plus any required base game content can override these
- `latest`: Loaded after third party game content (sfm, tf2, etc.)
        - Anything can override these

Create a named subfolder inside of one to automatically add it to the game's content search path.
