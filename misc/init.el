(setq inhibit-startup-message t)

(scroll-bar-mode -1) ; Disable visible scrollbar
(tool-bar-mode -1)   ; Disable the toolbar
(tooltip-mode -1)    ; Disable tooltips
(set-fringe-mode 10) ; Give some breathing room

(setq make-backup-files nil        ; disable ~ backups
      auto-save-default nil        ; disable #autosaves#
      create-lockfiles nil)        ; disable .#lockfiles

(add-hook 'emacs-startup-hook
          (lambda ()
            (split-window-right)))

(menu-bar-mode -1)   ; Disable the menu bar

;; Set up the visible bell
(setq visible-bell t)

(set-face-attribute 'default nil :font "Iosevka Term":height 110)
(setq-default line-spacing 0)
(set-face-attribute 'default nil :height 110) ;; 110 = 11pt, adjust as needed

(custom-set-faces
 ;; custom-set-faces was added by Custom.
 ;; If you edit it by hand, you could mess it up, so be careful.
 ;; Your init file should contain only one such instance.
 ;; If there is more than one, they won't work right.
 '(font-lock-comment-face ((t (:foreground "#666666"))))
 '(font-lock-string-face ((t (:foreground "#888888")))))

;; Make ESC quit prompts
(keymap-global-set "<escape>" 'keyboard-escape-quit)

;; Initialize package sources
(require 'package)

(setq package-archives '(("melpa" . "https://melpa.org/packages/")
                         ("org" . "https://orgmode.org/elpa/")
                         ("elpa" . "https://elpa.gnu.org/packages/")))

(package-initialize)
(unless package-archive-contents
 (package-refresh-contents))

;; Initialize use-package on non-Linux platforms
(unless (package-installed-p 'use-package)
   (package-install 'use-package))

(require 'use-package)
(setq use-package-always-ensure t)

(column-number-mode)
(global-display-line-numbers-mode t)

;; Disable line numbers for some modes
(dolist (mode '(org-mode-hook
		 term-mode-hook
		 shell-mode-hook
		 eshell-mode-hook))
  (add-hook mode (lambda () (display-line-numbers-mode 0))))

(use-package command-log-mode)

(use-package modus-themes
  :ensure t
  :config
  (setq modus-themes-disable-other-themes t
        modus-themes-bold-constructs nil
        modus-themes-italic-constructs nil
        modus-themes-underline nil)

  (load-theme 'modus-vivendi-tinted t)

  ;; Set gentle, low-contrast colors for some syntax faces
  (set-face-attribute 'font-lock-keyword-face nil
                      :foreground "#7f9f7f" ; soft greenish
                      :weight 'normal
                      :slant 'normal)
  (set-face-attribute 'font-lock-function-name-face nil
                      :foreground "#93a1a1" ; subtle cyan/gray
                      :weight 'normal
                      :slant 'normal)
  (set-face-attribute 'font-lock-type-face nil
                      :foreground "#b2b2b2" ; light gray
                      :weight 'normal
                      :slant 'normal)
  (set-face-attribute 'font-lock-constant-face nil
                      :foreground "#a0a0a0" ; muted gray
                      :weight 'normal
                      :slant 'normal)
  (set-face-attribute 'font-lock-variable-name-face nil
                      :foreground "#8f8f8f" ; medium gray
                      :weight 'normal
                      :slant 'normal)
  (set-face-attribute 'font-lock-string-face nil
                      :foreground "#9f9f7f" ; soft muted yellowish
                      :weight 'normal
                      :slant 'normal)
  (set-face-attribute 'font-lock-comment-face nil
                      :foreground "#6c6c6c" ; subtle gray
                      :slant 'italic))
(use-package ivy

  :diminish
  :bind (("C-s" . swiper)
         :map ivy-minibuffer-map
         ("TAB" . ivy-alt-done)	
         ("C-l" . ivy-alt-done)
         ("C-j" . ivy-next-line)
         ("C-k" . ivy-previous-line)
         :map ivy-switch-buffer-map
         ("C-k" . ivy-previous-line)
         ("C-l" . ivy-done)
         ("C-d" . ivy-switch-buffer-kill)
         :map ivy-reverse-i-search-map
         ("C-k" . ivy-previous-line)
         ("C-d" . ivy-reverse-i-search-kill))
  :init
  (ivy-mode 1))

(keymap-global-set "C-M-j" 'counsel-switch-buffer) 

(use-package all-the-icons
  :if (display-graphic-p))

(use-package doom-modeline
  :init (doom-modeline-mode 1)
  :custom ((doom-modeline-height 15)))

(use-package doom-themes)

;(use-package rainbow-delimiters
;  :hook (prog-mode . rainbow-delimiters-mode))

(use-package which-key
  :init (which-key-mode)
  :diminish which-key-mode
  :config
  (setq which-key-idle-delay 0.3))

(use-package ivy-rich
  :init
  (ivy-rich-mode 1))

(use-package counsel
  :diminish
  :bind (("M-x" . counsel-M-x)
	 ("C-x b" . counsel-ibuffer)
	 ("C-x C-f" . counsel-find-file)
	 :map minibuffer-local-map
	 ("C-r" . 'counsel-minibuffer-history))
  :config
  (setq ivy-initial-inputs-alist nil)) ;; Don't start searches with

(use-package helpful
  :custom
  (counsel-describe-function-function #'helpful-callable)
  (counsel-describe-variable-function #'helpful-variable)
  :bind
  ([remap describe-function] . counsel-describe-function)
  ([remap describe-command] . helpful-command)
  ([remap describe-variable] . counsel-describe-variable)
  ([remap describe-key] . helpful-key))


(use-package general
  :config
 ;(general-evil-setup )
 (general-create-definer rune/leader-keys
   :keymaps '(normal insert visual emacs)
   :prefix "SPC"
   :global-prefix "C-SPC")
   (rune/leader-keys
   "t" '(:ignore t :which-key "toggles")))

(use-package evil
  :init
  (setq evil-want-integration t)
  (setq evil-want-keybinding nil) 
  (setq evil-want-C-u-scroll t)
  (setq evil-want-C-i-jump nil)
  :config
  (evil-mode 1)
  (define-key evil-insert-state-map (kbd "C-g") 'evil-normal-state)
  (define-key evil-insert-state-map (kbd "C-h") 'evil-delete-backward-char-and-join)
  
  ;; Use visual line motions even outside of visual-line-mode buffers
  (evil-global-set-key 'motion "j" 'evil-next-visual-line)
  (evil-global-set-key 'motion "k" 'evil-previous-visual-line)

  (evil-set-initial-state 'messages-buffer-mode 'normal)
  (evil-set-initial-state 'dashboard-mode 'normal))

(use-package evil-collection
  :after evil
  :config
  (evil-collection-init))

(use-package evil-commentary
  :ensure t
  :config
  (evil-commentary-mode))

(use-package hydra)
(defhydra hydra-text-scale (:timeout 4)
	  "scale text"
	  ("j" text-scale-increase "in")
	  ("k" text-scale-decrease "out")
	  ("f" nil "finished" :exit t))


(use-package projectile
  :diminish projectile-mode
  :config (projectile-mode +1)
  :custom ((projectile-completion-system 'ivy))
  :bind-keymap
  ("C-c p" . projectile-command-map)
  :init
  (when (file-directory-p "~/dev/")
    (setq projectile-project-search-path '("~/dev/")))
  (setq projectile-switch-project-action #'projectile-dired))


(general-define-key
 :keymaps 'override
 :prefix "SPC"
 :states '(normal visual)
 "p" '(:keymap projectile-command-map :package projectile :which-key "projectile"))

(use-package counsel-projectile
  :after (projectile counsel)
  :config (counsel-projectile-mode))

(setq projectile-run-project-function
      (lambda () (projectile-run-command projectile-run-project-command)))

(use-package clang-format
  :ensure t)

;; Optional: Auto format on save for C/C++ files
(defun my/clang-format-buffer ()
  "Format buffer with clang-format if in C/C++ mode."
  (when (derived-mode-p 'c-mode 'c++-mode)
    (clang-format-buffer)))

;(add-hook 'before-save-hook #'my/clang-format-buffer)

;; Add manual formatting keybinding under SPC
(rune/leader-keys
  "c" '(:ignore t :which-key "code")
  "c f" '(clang-format-buffer :which-key "format buffer"))

(define-key evil-normal-state-map (kbd "<backtab>") 'evil-shift-left)
(define-key evil-visual-state-map (kbd "<backtab>") 'evil-shift-left)

(setq-default indent-tabs-mode nil) ; Use spaces instead of tabs
(setq-default tab-width 4)          ; But define tab width if you ever see a tab
(setq-default c-basic-offset 4)     ; For C/C++ and other languages

;; Define custom faces
(defface my-todo-face
  '((t (:foreground "OrangeRed" :weight bold)))
  "Face for TODO keywords.")

(defface my-fixme-face
  '((t (:foreground "Red" :weight bold)))
  "Face for FIXME keywords.")

(defface my-note-face
  '((t (:foreground "DarkGreen" :weight bold)))
  "Face for NOTE keywords.")

;; Add font-lock rules to prog-mode
(add-hook 'prog-mode-hook
          (lambda ()
            (font-lock-add-keywords nil
             '(("\\bTODO\\b\\(:\\)?" 0 'my-todo-face t)
               ("\\bFIXME\\b\\(:\\)?" 0 'my-fixme-face t)
               ("\\bNOTE\\b\\(:\\)?" 0 'my-note-face t)))))

(setq-default abbrev-mode t)               ;; Enable abbrevs globally
(setq save-abbrevs t)                      ;; Save them between sessions
(setq abbrev-file-name "~/.emacs.d/abbrev_defs") ;; File to store abbrevs
(when (file-exists-p abbrev-file-name)
  (quietly-read-abbrev-file abbrev-file-name))

(define-abbrev-table 'global-abbrev-table
  '(("todo"  "TODO(Sebas): " nil 0)
    ("note"  "NOTE(Sebas): " nil 0)
    ("fixme" "FIXME(Sebas): " nil 0)))
(defun my-c-style ()
  "Custom C/C++ style for Handmade Hero-inspired projects."
  (interactive)

  ;; Base style
  (c-set-style "stroustrup")

  ;; Indentation settings
  (setq c-basic-offset 4
        tab-width 4
        indent-tabs-mode nil) ;; Use spaces

  ;; Allman-like brace style
  (setq c-hanging-braces-alist
        '((class-open . before)
          (inline-open . before)
          (namespace-open . before)
          (block-open . before)
          (extern-lang-open . before)
          (function-open . before)
          (statement-case-open . before)
          (substatement-open . before)
          (brace-list-open . before)
          (catch-open . before)
          (else-open . before)))

  ;; Indentation behavior
  (setq c-offsets-alist
      '((innamespace . 0)
        (cpp-macro . 0)
        (case-label . +)
        (arglist-intro . +)
        (arglist-close . 0)
        (topmost-intro . 0)
        (substatement . +)
        (substatement-open . 0)
        (statement-block-intro . +)
        (block-open . 0)))
  ;; Hungry delete
  (c-toggle-hungry-state 1)


;; Argument alignment (approximate)
  (setq c-indent-function-args 'align))

  (add-hook 'c-mode-common-hook 'my-c-style)

;; Preprocessor highlighting


(defun my-c-pp-indent-fix ()
  (when (and (derived-mode-p 'c-mode 'c++-mode)
             (save-excursion
               (beginning-of-line)
               (looking-at "^\\s-*#")))
    0))

  (add-hook 'c-mode-common-hook
          (lambda ()
            (add-hook 'c-special-indent-hook #'my-c-pp-indent-fix nil t)))

(electric-pair-mode 1)
(electric-indent-mode 1)
(defun my/smart-semicolon ()
  "Insert a semicolon at the end of the line unless one already exists."
  (interactive)
  (save-excursion
    (end-of-line)
    (unless (save-excursion
              (skip-chars-backward " \t")
              (eq (char-before) ?\;))
      (insert ";")))
  (end-of-line))

;; Bind it in insert mode
(define-key evil-insert-state-map (kbd "M-;") #'my/smart-semicolon)
(with-eval-after-load 'cc-mode
  (define-key evil-insert-state-map (kbd "TAB") #'c-indent-line-or-region))
(custom-set-variables
 ;; custom-set-variables was added by Custom.
 ;; If you edit it by hand, you could mess it up, so be careful.
 ;; Your init file should contain only one such instance.
 ;; If there is more than one, they won't work right.
 '(package-selected-packages
   '(all-the-icons command-log-mode counsel counsel-projectile
		   doom-modeline doom-themes evil evil-collection
		   general helpful hydra ivy ivy-rich modus-themes
		   projectile rainbow-delimiters which-key)))

