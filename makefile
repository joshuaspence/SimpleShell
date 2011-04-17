##################################################################################################
# Makefile for building the program  'myshell'
#-------------------------------------------------------------------------------------------------
# Author:	Joshua Spence
# SID:		308216350
#=================================================================================================
# Targets are:
#    myshell - create the program  'myshell'.
#    clean - remove all object files, temporary files, backup files, striped files, target executable and tar files.
#	 partial-clean - same as clean but doesn't remove striped files.
#	 debug - create the debug version of 'myshell' with capability to output useful debug information.
#	 tar - create a tar file containing all files currently in the directory.
#	 strip - strip unused #ifdef statements from source code (project must be MADE first using a separate make statement).
#	 restore-backup - used to recover from a failed stripcc call.
#	 help - display the help file for instructions on how to make this project.
##################################################################################################

CC = gcc
CFLAGS = -W -Wall -std=c99 -pedantic -c
LDFLAGS = -W -Wall -std=c99 -pedantic
CFLAGS_DEBUG = -DDEBUG -g

SRCDIR = src
SRCDIR_BACKUP = src/backup
SRCDIR_STRIPED = src/striped
INCDIR = inc
INCDIR_BACKUP = inc/backup
INCDIR_STRIPED = inc/striped
OBJDIR = obj

STRIPCC_PATH = ./tools/stripcc/stripcc
STRIPCC_ERROR_FILE = stripcc.err
TAR_FILE = Assignment1_308216350.tar

DEST = myshell
FILES = myshell cmd_internal utility
OBJS = $(FILES:%=$(OBJDIR)/%.o)
INCS = $(FILES:%=$(INCDIR)/%.h) $(INCDIR)/strings.h
SRCS = $(FILES:%=$(SRCDIR)/%.c)

# create the program  'myshell'
$(DEST): $(OBJS)
	@echo "====================================================="
	@echo "Linking the target $@"
	@echo "====================================================="
	$(CC) $(LDFLAGS) $^ -o $@
	@echo "------------------- Link finished -------------------"
	@echo

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(INCDIR)/%.h
	@echo "====================================================="
	@echo "Compiling $<"
	@echo "====================================================="
# create OBJDIR if it doesn't exist
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) $< -o $(OBJDIR)/$*.o
	@echo "--------------- Compilation finished ----------------"
	@echo

$(SRCDIR)/myshell.c: $(INCDIR)/cmd_internal.h $(INCDIR)/utility.h $(INCDIR)/strings.h
$(SRCDIR)/cmd_internal.c: $(INCDIR)/utility.h $(INCDIR)/strings.h
$(SRCDIR)/utility.c: $(INCDIR)/strings.h

# the following targets are phony
.PHONY: clean partial-clean help strip restore-backup

# remove all object files, temporary files, backup files, striped files, target executable and tar files
clean:
	@echo "====================================================="
	@echo "Cleaning directory."
	@echo "====================================================="
	rm -rfv $(OBJDIR)/*.o *~ $(INCDIR)/*~ $(INCDIR_BACKUP) $(INCDIR_STRIPED) $(SRCDIR)/*~ $(SRCDIR_BACKUP) $(SRCDIR_STRIPED) $(DEST) $(TAR_FILE) $(STRIPCC_ERROR_FILE)
	@echo "------------------ Clean finished -------------------"
	@echo

# same as clean but doesn't remove striped files
partial-clean:
	@echo "====================================================="
	@echo "Partial cleaning directory."
	@echo "====================================================="
	rm -rfv $(OBJDIR)/*.o *~ $(INCDIR)/*~ $(INCDIR_BACKUP) $(SRCDIR)/*~ $(SRCDIR_BACKUP) $(DEST) $(TAR_FILE) $(STRIPCC_ERROR_FILE)
	@echo "------------------ Clean finished -------------------"
	@echo

# create a tar file containing all files currently in the directory
tar:
	@echo "====================================================="
	@echo "Creating tar file."
	@echo "====================================================="
# delete existing tar file
	@rm -f $(TAR_FILE)
	tar cfv $(TAR_FILE) ./*
	@echo "-------------- Tar creation finished ----------------"
	@echo

# display the help file for instructions on how to make this project
help:
	@echo "=========================================================================================================="
	@echo "Makefile for building the program  'myshell'"
	@echo "=========================================================================================================="
	@echo "Targets are:"
	@echo "    myshell              create the program  'myshell'."
	@echo "    clean                remove all object files, temporary files, backup files, striped files, target executable and tar files."
	@echo "    partial-clean        same as clean but doesn't remove striped files."
	@echo "    debug                create the debug version of 'myshell' with capability to output useful debug information."
	@echo "    tar                  create a tar file containing all files currently in the directory."
	@echo "    strip                strip unused #ifdef statements from source code (project must be MADE first using a separate make statement)."
	@echo "    restore-backup       used to recover from a failed stripcc call."
	@echo "    help                 display the help file for instructions on how to make this project."
	@echo
	@echo "Use:"
	@echo "    make                 create program 'myshell'."
	@echo "    make myshell         same as make."
	@echo "    make myshell && make strip"
	@echo "                         create program 'myshell' and then create striped source files."
	@echo "    make debug           create program 'myshell' with capability to output useful debug information."
	@echo "    make debug && make strip"
	@echo "                         create program 'myshell' with capability to output useful debug information and then create striped source files."
	@echo "    make clean           remove all object files, temporary files, backup files, striped files, target executable and tar files."
	@echo "    make myshell && make strip partial-clean tar"
	@echo "                         create a tar file containing the files required for assignment submission."
	@echo "    make help            display the help file."
	@echo "----------------------------------------------------------------------------------------------------------"
	@echo
	
# create the debug version of 'myshell' with capability to output useful debug information
debug: CFLAGS += $(CFLAGS_DEBUG)
debug: $(DEST)

# strip unused ifdef statements from source code (project must be MADE first)
strip:
	@echo "====================================================="
	@echo "Striping unused ifdef statements from *.c and *.h "
	@echo "files"
	@echo "====================================================="
	@echo "Removing existing striped files."
	-rm -rf $(SRCDIR_STRIPED) $(INCDIR_STRIPED)
	@echo "-----------------------------------------------------"
	@echo "Removing existing backup files."
	-@rm -rf $(SRCDIR_BACKUP) $(INCDIR_BACKUP)
	@echo "-----------------------------------------------------"
	@echo "Creating striped directories."
	mkdir -p $(SRCDIR_STRIPED) 1>/dev/null 2>&1
	mkdir -p $(INCDIR_STRIPED) 1>/dev/null 2>&1
	@echo "-----------------------------------------------------"
	@echo "Creating backup directories."
	mkdir -p $(SRCDIR_BACKUP) 1>/dev/null 2>&1
	mkdir -p $(INCDIR_BACKUP) 1>/dev/null 2>&1
	@echo "-----------------------------------------------------"
	@echo "Backing up original files."
	cp -p $(SRCDIR)/*.c $(SRCDIR_BACKUP)/
	cp -p $(INCDIR)/*.h $(INCDIR_BACKUP)/
	@echo "-----------------------------------------------------"
	@echo "Running stripcc."
	$(STRIPCC_PATH)
	@echo "-----------------------------------------------------"
	@echo "Moving striped files."
	mv $(SRCDIR)/*.c $(SRCDIR_STRIPED)/
	mv $(INCDIR)/*.h $(INCDIR_STRIPED)/
	@echo "-----------------------------------------------------"
	@echo "Moving backup files."
	cp $(SRCDIR_BACKUP)/*.c $(SRCDIR)/
	cp $(INCDIR_BACKUP)/*.h $(INCDIR)/
	@echo "-----------------------------------------------------"
	@echo "Deleting backup directories."
	-rm -rf $(SRCDIR_BACKUP) $(INCDIR_BACKUP)
	@echo "----------------- Striping finished -----------------"
	@echo

# restore backed up source code files in case stripcc failed
restore-backup:
	@echo "====================================================="
	@echo "Restoring backup files"
	@echo "====================================================="
	@echo "Removing striped directories."
	-rm -rfv $(SRCDIR_STRIPED) $(INCDIR_STRIPED)
	@echo "-----------------------------------------------------"
	@echo "Removing corrupted files."
	-rm -fv $(SRCDIR)/*.c* $(INCDIR)/*.h*
	@echo "-----------------------------------------------------"
	@echo "Restoring backup files."
	cp -p $(SRCDIR_BACKUP)/*.c $(SRCDIR)/
	cp -p $(INCDIR_BACKUP)/*.h $(INCDIR)/
	@echo "-----------------------------------------------------"
	@echo "Removing backup directories."
	-rm -rf $(SRCDIR_BACKUP) $(INCDIR_BACKUP)
	@echo "----------------- Striping finished -----------------"
	@echo