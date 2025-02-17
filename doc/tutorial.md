Suika2 Tutorial
================

Welcome to the world of Suika2!

This document explains how to make a visual novel using Suika2.
It's very easy.

Ready? Let's get started!

## Preparation

We'll use Windows here.

Firstly, [download](https://suika2.com/en/) the Suika2 binary.

Extract the downloaded ZIP file. We see the following file and folders.
`suika.exe` is the Sukia2 executable file. Other folders include a sample
game.

* `suika2/`
    * `bg/` A folder to store background image files.
    * `bgm/` A folder to store background music files.
    * `cg/` A folder to store system image files such as for message boxes.
    * `ch/` A folder to store character image files.
    * `conf/` A folder to store setting file.
    * `cv/` A folder to store voice files.
    * `font/` A folder to store font files.
    * `se/` A folder to store sound effect files.
    * `tool/` A folder that includes the file packager.
    * `txt/` A folder to store script files.
    * `suika.exe` Executable application file for Windows.

This tutorial makes changes to the sample game.

## Enable English mode

* Open `conf/config.txt` by a text editor.
* You can find the following line:
```
#language=English
```
* Delete the heading character `#` and save the file.

## Open script file

We can find a file named `init.txt` in the `txt` folder.
This file is the first script file to be loaded after Suika2 starts.
If you open this file with a text editor, you'll see that the script file is
just a plain-text file.

## Show background image

For our first step, we'll change the background image.
This is the most basic command.

Replace the contents of `init.txt` as follows:

```
@bg roof.png 1.0
@click
```

After saving it, run `suika.exe`.
Suika2 will show a roof terrace, wait for a click, and then exit.

`@bg` is the command to show a background image.
`roof.png` is a file inside the `bg` folder.
`1.0` is the fade-in time counted in seconds.

## Show character image

Showing character images is mostly the same as the `@bg` command.
Replace the contents of `init.txt` as follows:

```
@bg roof.png 1.0
@ch center 001-fun.png 1.0
@click
```

After saving it, run `suika.exe`.
Suika2 will show a roof terrace, a character, wait for a click, and then exit.

`@ch` is the command to show a character.
`center` is the horizontal position to show a character.
`001-fun.png` is a file inside the `ch` folder.
`1.0` is the fade-in time counted by seconds.

## Show message

Showing dialogue is fundamental to visual novels.
You can print text in the message box.
Replace the contents of `init.txt` as follows:

```
@bg roof.png 1.0
@ch center 001-fun.png 1.0
Hi, my name is Midori.
*Midori*Message can be shown with character name.
*Midori*011.ogg*Message can be shown with voice.
```

After saving it, run `suika.exe`.
Suika2 will show a roof terrace, a character, three messages, and then exit.

Any script lines which don't start with @, : or \* are recognized as messages.

## Play BGM

You can play BGM using the `@bgm` command.
Replace the contents of `init.txt` as follows:

```
@bgm 01.ogg
@bg roof.png 1.0
@ch center 001-fun.png 1.0
Playing BGM.
```

After saving it, run `suika.exe`.
Suika2 will play music.
`@bgm` is the command to play BGM.
`01.ogg` is a file inside the `bgm` folder.

Suika2 can only play sound files encoded using Ogg Vorbis 44.1kHz stereo or monaural format.

## Show options

A game may have options for multiple endings.
Replace the contents of `init.txt` as follows:

```
@bg roof.png 1.0
@ch center 001-fun.png 1.0
Ok, I'm gonna go to school.
@choose label1 "By foot." label2 "By bicycle." label3 "I decided not to."
:label1
I'm gonna go to school by foot.
@goto end
:label2
I'm gonna go to school by bicycle.
@goto end
:label3
I'm home sick from school.
:end
```

After saving it, run `suika.exe`.
Suika2 will show three options.
`@choose` is the command to show options.
`label1`, `label2` and `label3` are jump destinations.
Option messages follow labels.

`:label1` is a label to specify a jump destination.
Label lines are ignored when they are executed.

## Set a flag and branch

Next, we set a flag and create a branch.
Replace the contents of `init.txt` as follows:

```
@bg roof.png 1.0
@ch center 001-fun.png 1.0
Ok, I'm gonna go to school.
@choose label1 "By foot." label2 "By bicycle." label3 "I decided not to."
:label1
I'm gonna go to school by foot.
@set $1 = 1
@goto next
:label2
I'm gonna go to school by bicycle.
@goto next
:label3
I'm home sick from school.
:next
@if $1 == 0 end
I found money on the ground.
:end
```

After saving it, run `suika.exe`.
Suika2 will show three options.
If you choose "By foot.", the message "I found money on the ground." is shown.

Here, we use a combination of `@set` and `@if`.

`@set` is the command to set variables.
`$1` on the LHS means we store to variable at number 1.
`=` means simple assignment.
Lastly, `1` means integer 1.

All variables are initially zero.
`$1` is set to `1` only when "By foot." is selected.

`@if` is the command to jump by condition.
`$1 == 0` means `when variable at number 1 is 0`.
`end` is the jump destination label.

## Show menu

You can make a menu screen.
Replace the contents of `init.txt` as follows:

```
@menu menu.png menu_selected.png START 2 495 314 192 LOAD 316 494 322 203 CONFIG 640 492 317 204 QUIT 960 497 317 201
:START
:LOAD
:CONFIG
:QUIT
```

After saving it, run `suika.exe`.
Suika2 will show the title screen of the sample game.

Let's look at this in more detail:

`@menu` is the command to show the menu screen. It uses two images.
`@menu menu.png menu_selected.png` means we use `menu.png` and `menu_selected.png` in the `bg` folder.
Compare these images.
`menu.png` is an image for when the button isn't being pointed at by the mouse.
`menu_selected.png` is an image for when the button is being pointed at by the mouse.

`START 2 495 314 192` creates a button.
`START` is the jump destination label.
`2 495 314 192` means creating a button with 314x192 size at position (2, 495).

## Split script file

It is difficult for authors to maintain a single large script file.
Hence, we have a way to split script files.

Replace the contents of `init.txt` as follows:

* init.txt
```
@bg roof.png 1.0
@ch center 001-fun.png 1.0
inside init.txt
@load second.txt
```

Create `second.txt` in the `txt` folder as follows.

* second.txt
```
moved to second.txt
```

After saving these files, run `suika.exe`.
Suika2 will show "moved to second.txt" after "inside init.txt".

`@load` is the command to jump to a specified script file.

## Create package

We can create a single package file to contain scripts, images, sounds and so
on. Follow these steps:

1. Double click `suika-pro.exe`. `Suika2 Pro for Creators` will open.
2. Click `File` in the menu, then click `Export package`. It will create a file, `data01.arc`.
3. We should only distribute `suika.exe` and `data01.arc`.

When we have both the package and a normal file, Suika2 uses the normal file.
