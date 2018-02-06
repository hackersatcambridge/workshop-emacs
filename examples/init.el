;This tells Emacs to use MELPA
(require 'package)
;;Optionally you can use melpa stable instead of melpa 
;(add-to-list 'package-archives
;             '("melpa-stable" . "https://stable.melpa.org/packages/"))
(add-to-list 'package-archives
	     '("melpa" . "http://melpa.milkbox.net/packages/"))
(package-initialize)

;This variable can hold packages you want to always install
;I am using it to ensure everyone can easily get better defaults and potentially more
(defvar myPackages
  '(better-defaults
    ))

;Use below snippet to install packages in variable myPackages
(mapc #'(lambda (package)
    (unless (package-installed-p package)
      (package-install package)))
      myPackages)

;(setq inhibit-startup-message t) ;; hide the startup message
(global-linum-mode t) ;; enable line numbers globally

;;The following section requires packages to be installed.
;;Use M-x package-install RET <package-name> RET 

;;Requires package solarized-theme
;(load-theme 'solarized-dark t) ;;Way better than default theme, feel free to pick your own preference though!


;;Enable Ivy and some extras for it
;:Requires package ivy, counsel, and flx
;(require 'ivy)
;(ivy-mode 1)
;(counsel-mode 1)
;(setq ivy-count-format ""
;      ivy-display-style nil
;      ivy-minibuffer-faces nil)
;(require 'flx)
;(setq ivy-re-builders-alist '((t . ivy--regex-fuzzy)))
