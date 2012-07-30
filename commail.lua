-- commail.lua - communal mailbox for etpub
-- this module implements a server side mailbox system intended to give players more fexibility in providing server related feedback, e.g. maprotation, complaints, teamkills etc.
-- 
-- script by fart11eleven - nmueller1 at gmx dot net
--

SCRIPTVERSION = "0.1"
MODSUPPORT = "etpub"
MODULENAME = "Communal Mailbox"

-------------------------------------------------------------------------------------
-------------------------------CONFIG START------------------------------------------

MAILBOX_FILENAME = "commail.log"
MAILBOX_MAX_FILESIZE = 8192 -- in bytes

CMD_MSG_WRITE = "!msgwrite"
CMD_MSG_DELETE = "!msgdelete"
CMD_MSG_DELETE_PAGE = "!msgdeletepage"
CMD_MSG_DELETE_ALL = "!msgdeleteall"
CMD_MSG_LIST = "!msglist"
CMD_MSG_SIZE = "!msgsize"

SHRUBLEVEL_CMD_MSG_WRITE = 2
SHRUBLEVEL_CMD_MSG_DELETE = 5
SHRUBLEVEL_CMD_MSG_DELETE_PAGE = 5
SHRUBLEVEL_CMD_MSG_DELETE_ALL = 5
SHRUBLEVEL_CMD_MSG_LIST = 5
SHRUBLEVEL_CMD_MSG_SIZE = 5

SPAM_PROTECTION = 1
RCV_TIMEOUT = 20000 -- in milliseconds

AUTO_CLEANUP = 1
AUTO_CLEANUP_MSG_OLDER = 30 -- in days

-------------------------------CONFIG END--------------------------------------------
-------------------------------------------------------------------------------------

last_message_time = 0

mailbox = {}
mailbox.msg = {}
mailbox.filesize = 0

function et_InitGame( levelTime, randomSeed, restart )

   et.RegisterModname( MODULENAME .. ":" .. SCRIPTVERSION .. ":" .. MODSUPPORT )

   local fd, len = et.trap_FS_FOpenFile( MAILBOX_FILENAME, et.FS_READ )
   if len == -1 then
      et.G_Print( string.format("%s: cannot open %s\n", MODULENAME, MAILBOX_FILENAME) )
      et.trap_FS_FCloseFile( fd )
      return 0
   end

   if len == 0 then
      et.trap_FS_FCloseFile( fd )
      return 0
   end

   local filedata = et.trap_FS_Read( fd, len )
   et.trap_FS_FCloseFile( fd )
   
   local newlinepos, parsefiledata, findmsg, msghead, msgtail, msgnum = 0, true, true, nil, nil, 1

   while ( parsefiledata == true ) do

      newlinepos, newlinepos = string.find (filedata, "\n", newlinepos + 1)
      if newlinepos == nil then
         parsefiledata = false
      end

      findmsg = true

      while ( findmsg == true and parsefiledata == true ) do
         msghead = newlinepos
         newlinepos, newlinepos = string.find (filedata, "\n", newlinepos + 1)
         if newlinepos == nil then
            parsefiledata = false
         end

         if msghead + 1 ~= newlinepos and parsefiledata == true then
            findmsg = false
            msgtail = newlinepos
         end
      end

      if parsefiledata == true then
         mailbox.msg[msgnum] = {}
         mailbox.msg[msgnum].rawstring = string.sub(filedata, msghead, msgtail)
         mailbox.msg[msgnum].rawsize = string.len(mailbox.msg[msgnum].rawstring)
         mailbox.filesize = mailbox.filesize + mailbox.msg[msgnum].rawsize
         msgnum = msgnum + 1
      end     
   end

end

function et_ShutdownGame( restart )

   if AUTO_CLEANUP == 1 then
      AutoCleanUp()
   end

   local filedata = ""

   for i = 1, #mailbox.msg do
      filedata = filedata .. mailbox.msg[i].rawstring
   end

   local fd, len = et.trap_FS_FOpenFile( MAILBOX_FILENAME, et.FS_WRITE )
   if len == -1 then
      et.G_Print( string.format("%s: cannot open %s\n", MODULENAME, MAILBOX_FILENAME) )
      return 0
   end

   local count = et.trap_FS_Write( filedata, string.len(filedata), fd )
   if count <= 0 then
      et.G_Print( string.format("%s: cannot write %s.\n", MODULENAME, MAILBOX_FILENAME) )
   end

   et.trap_FS_FCloseFile( fd )
end

function et_ClientCommand( clientNum, command )

   if command ~= "say" then
      return 0
   end

   if et.trap_Argv(1) == CMD_MSG_WRITE then

      local level = et.G_shrubbot_level( clientNum )
      if level < SHRUBLEVEL_CMD_MSG_WRITE then
         et.trap_SendServerCommand( clientNum, string.format("cpm \"%s: denied. Level %d needed.\n\"", MODULENAME, SHRUBLEVEL_CMD_MSG_WRITE) )
         return 0
      end

      local args = et.ConcatArgs( 2 )
      local argslen = string.len(args)
      if argslen == 0 then
         et.trap_SendServerCommand( clientNum, string.format("cpm \"%s: message not filed. Missing arguments.\n\"", MODULENAME) )
         return 0
      end

      if argslen > 800 then
         et.trap_SendServerCommand( clientNum, string.format("cpm \"%s: message not filed (too big).\n\"", MODULENAME) )
         return 0
      end

      if SPAM_PROTECTION == 1 then
         millisec = et.trap_Milliseconds()
         if millisec - last_message_time < RCV_TIMEOUT then         
            et.trap_SendServerCommand( clientNum, string.format("cpm \"%s: message not filed. Please wait ^1%d^7 seconds.\n\"", MODULENAME, (RCV_TIMEOUT - (millisec - last_message_time))/1000) )
            return 0
         end

         last_message_time = millisec
      end

      local date = os.date()     
      local userinfo = et.trap_GetUserinfo( clientNum )
      local name = et.Info_ValueForKey( userinfo, "name" )
      local guid = et.Info_ValueForKey( userinfo, "cl_guid" )
      if string.len(guid) == 32 then
         guid = string.sub(guid, 1, 24)
      else
         et.trap_SendServerCommand( clientNum, string.format("cpm \"%s: message not filed. Cannot verify guid.\n\"", MODULENAME) )
         return 0
      end

      local msg = string.format("\n%s %s %s MSG: %s\n", date, guid .. "...", et.Q_CleanStr( name ), et.Q_CleanStr( args ))
      local msglen = string.len(msg)
      local newfilesize = mailbox.filesize + msglen

      if newfilesize > MAILBOX_MAX_FILESIZE then
         et.trap_SendServerCommand( clientNum, string.format("cpm \"%s: message not filed. Box too full.\n\"", MODULENAME) )
         et.G_Print( string.format("%s: attempt to write to %s denied. Maxfilesize %d bytes reached.\n", MODULENAME, MAILBOX_FILENAME, MAILBOX_MAX_FILESIZE) )
         return 0
      end

      local msgnum = #mailbox.msg + 1
      mailbox.msg[msgnum] = {}
      mailbox.msg[msgnum].rawstring = msg
      mailbox.msg[msgnum].rawsize = msglen
      mailbox.filesize = newfilesize

      et.trap_SendServerCommand( clientNum, string.format("cpm \"%s: message filed.\n\"", MODULENAME) )
      et.G_Print( string.format("%s: message filed.\n", MODULENAME) )
      return 0
   end

   if et.trap_Argv(1) == CMD_MSG_DELETE then

      local level = et.G_shrubbot_level( clientNum )
      if level < SHRUBLEVEL_CMD_MSG_DELETE then
         et.trap_SendServerCommand( clientNum, string.format("cpm \"%s: denied. Level %d needed.\n\"", MODULENAME, SHRUBLEVEL_CMD_MSG_DELETE) )
         return 0
      end

      if mailbox.filesize == 0 then
         et.trap_SendServerCommand( clientNum, string.format("cpm \"%s: empty. Nothing to delete.\n\"", MODULENAME) )
         return 0
      end

      local cmdline_delstring = et.ConcatArgs( 2 )
      if string.len(cmdline_delstring) == 0 then
         et.trap_SendServerCommand( clientNum, string.format("cpm \"%s: missing argument.\n\"", MODULENAME) )
         return 0
      end

      local a, b, numdeleted = 1, 1, 0
      for i = #mailbox.msg, 1, -1 do
         a, b  = string.find(mailbox.msg[i].rawstring, cmdline_delstring, 1)
         if a ~= nil then
            mailbox.filesize = mailbox.filesize - mailbox.msg[i].rawsize
            table.remove(mailbox.msg, i)
            numdeleted = numdeleted + 1
         end
      end

      et.trap_SendServerCommand( clientNum, string.format("cpm \"%s: %d messages to trash.\n\"", MODULENAME, numdeleted) )
      et.G_Print( string.format("%s: %d messages to trash.\n", MODULENAME, numdeleted) )
      return 0
   end

   if et.trap_Argv(1) == CMD_MSG_DELETE_PAGE then

      local level = et.G_shrubbot_level( clientNum )
      if level < SHRUBLEVEL_CMD_MSG_DELETE_PAGE then
         et.trap_SendServerCommand( clientNum, string.format("cpm \"%s: denied. Level %d needed.\n\"", MODULENAME, SHRUBLEVEL_CMD_MSG_DELETE_PAGE) )
         return 0
      end

      if mailbox.filesize == 0 then
         et.trap_SendServerCommand( clientNum, string.format("cpm \"%s: box is empty.\n\"", MODULENAME) )
         return 0
      end

      local page = {}
      local i, condition, pagenum, currentpagesize, createpage, upperlimitforpage = 1, true, 0, 0, false, 900

      while ( condition ) do

         if createpage == false then
            pagenum = pagenum + 1
            page[pagenum] = {}
            page[pagenum].pagehead = i
            currentpagesize = 0
            createpage = true
         end

         currentpagesize = currentpagesize + mailbox.msg[i].rawsize

         if currentpagesize > upperlimitforpage then
            page[pagenum].pagetail = i - 1
            page[pagenum].pagesize = currentpagesize - mailbox.msg[i].rawsize
            i = i - 1
            createpage = false
         end

         if i == #mailbox.msg then
            if createpage == true then
               page[pagenum].pagetail = i
               page[pagenum].pagesize = currentpagesize
            else
               pagenum = pagenum + 1
               page[pagenum] = {}
               page[pagenum].pagehead = i
               page[pagenum].pagetail = i
               page[pagenum].pagesize = mailbox.msg[i].rawsize
            end
            condition = false
         end

         i = i + 1
      end

      local totalnumpages = pagenum
      local pagerequested = tonumber(et.trap_Argv(2))

      if pagerequested == nil then
         et.trap_SendServerCommand( clientNum, string.format("print \"%s: no valid argument.\n\"", MODULENAME) )
         return 0
      else
         if pagerequested > 0 and pagerequested <= totalnumpages then
         
            local i, numdeleted = page[pagerequested].pagetail, 0
            while i >= page[pagerequested].pagehead do               
               table.remove(mailbox.msg, i)
               numdeleted = numdeleted + 1
               i = i - 1
            end
            mailbox.filesize = mailbox.filesize - page[pagerequested].pagesize
            et.trap_SendServerCommand( clientNum, string.format("cpm \"%s: page %d deleted (%d bytes).\n\"", MODULENAME, pagerequested, page[pagerequested].pagesize) )
         else
            et.trap_SendServerCommand( clientNum, string.format("cpm \"%s: page %d out of range %d - %d.\n\"", MODULENAME, pagerequested, 1, totalnumpages) )
         end
      end
      return 0
   end

   if et.trap_Argv(1) == CMD_MSG_DELETE_ALL then

      local level = et.G_shrubbot_level( clientNum )
      if level < SHRUBLEVEL_CMD_MSG_DELETE_ALL then
         et.trap_SendServerCommand( clientNum, string.format("cpm \"%s: denied. Level %d needed.\n\"", MODULENAME, SHRUBLEVEL_CMD_MSG_DELETE_ALL) )
         return 0
      end

      if mailbox.filesize == 0 then
         et.trap_SendServerCommand( clientNum, string.format("cpm \"%s: empty. Nothing to delete.\n\"", MODULENAME) )
      else
         local numdeleted, filediff, nummessages = 0, 0, #mailbox.msg
         for i = nummessages, 1, -1 do
            filediff = filediff + mailbox.msg[i].rawsize
            table.remove(mailbox.msg, i)
            numdeleted = numdeleted + 1
         end
         mailbox.filesize = mailbox.filesize - filediff
         et.trap_SendServerCommand( clientNum, string.format("cpm \"%s: %d bytes gone (%d messages).\n\"", MODULENAME, filediff, nummessages) )
         et.G_Print( string.format("%s: %d bytes gone.\n", MODULENAME, filediff) )
      end
      return 0
   end

   if et.trap_Argv(1) == CMD_MSG_LIST then

      local level = et.G_shrubbot_level( clientNum )
      if level < SHRUBLEVEL_CMD_MSG_LIST then
         et.trap_SendServerCommand( clientNum, string.format("cpm \"%s: denied. Level %d needed.\n\"", MODULENAME, SHRUBLEVEL_CMD_MSG_LIST) )
         return 0
      end

      if mailbox.filesize == 0 then
         et.trap_SendServerCommand( clientNum, string.format("cpm \"%s: box is empty.\n\"", MODULENAME) )
         return 0
      end

      local page = {}
      local i, condition, pagenum, currentpagesize, createpage, upperlimitforpage = 1, true, 0, 0, false, 900

      while ( condition ) do

         if createpage == false then
            pagenum = pagenum + 1
            page[pagenum] = {}
            page[pagenum].pagehead = i
            currentpagesize = 0
            createpage = true
         end

         currentpagesize = currentpagesize + mailbox.msg[i].rawsize

         if currentpagesize > upperlimitforpage then
            page[pagenum].pagetail = i - 1
            page[pagenum].pagesize = currentpagesize - mailbox.msg[i].rawsize
            i = i - 1
            createpage = false
         end

         if i == #mailbox.msg then
            if createpage == true then
               page[pagenum].pagetail = i
               page[pagenum].pagesize = currentpagesize
            else
               pagenum = pagenum + 1
               page[pagenum] = {}
               page[pagenum].pagehead = i
               page[pagenum].pagetail = i
               page[pagenum].pagesize = mailbox.msg[i].rawsize
            end
            condition = false
         end

         i = i + 1
      end

      local totalnumpages = pagenum
      local pagerequested = tonumber(et.trap_Argv(2))

      if pagerequested == nil then
         et.trap_SendServerCommand( clientNum, string.format("print \"%s: total number of %d pages (%d bytes).\n\"", MODULENAME, totalnumpages, mailbox.filesize) )
         return 0
      else
         if pagerequested > 0 and pagerequested <= totalnumpages then
            local rawpagecontent = ""
            for i = page[pagerequested].pagehead, page[pagerequested].pagetail do
               rawpagecontent = rawpagecontent .. mailbox.msg[i].rawstring
            end

            local msg = string.format("print \"\n%s: Page: %d/%d (%d bytes)\n%s\n\"", MODULENAME, pagerequested, totalnumpages, page[pagerequested].pagesize, rawpagecontent)
            et.trap_SendServerCommand( clientNum, msg )
            return 0
         else
            et.trap_SendServerCommand( clientNum, string.format("cpm \"%s: page %d out of range %d - %d.\n\"", MODULENAME, pagerequested, 1, totalnumpages) )
         end
      end
     
      return 0
   end

   if et.trap_Argv(1) == CMD_MSG_SIZE then

      local level = et.G_shrubbot_level( clientNum )
      if level < SHRUBLEVEL_CMD_MSG_SIZE then
         et.trap_SendServerCommand( clientNum, string.format("cpm \"%s: denied. Level %d needed.\n\"", MODULENAME, SHRUBLEVEL_CMD_MSG_SIZE) )
         return 0
      end
     
      et.trap_SendServerCommand( clientNum, string.format("cpm \"%s: %d bytes used. %d bytes free.\n\"", MODULENAME, mailbox.filesize, MAILBOX_MAX_FILESIZE - mailbox.filesize) )
      return 0
   end

end

function AutoCleanUp()

   if mailbox.filesize == 0 then
      return 0
   end

   local currenttime = os.date("*t")
   local cleanuptime = AUTO_CLEANUP_MSG_OLDER * 86400

   local numdeleted = 0
   for i = #mailbox.msg, 1, -1 do
     
      local msgtime = { }
      msgtime.month = tonumber(string.sub(mailbox.msg[i].rawstring, 2, 3))
      msgtime.day = tonumber(string.sub(mailbox.msg[i].rawstring, 5, 6))
      msgtime.year = 2000 + tonumber(string.sub(mailbox.msg[i].rawstring, 8, 9))

      if (os.difftime(os.time(currenttime), os.time(msgtime))) > cleanuptime then
         et.G_Print( string.format("%s: auto cleaned %s\n", MODULENAME, string.sub(mailbox.msg[i].rawstring, 1, 17)) )
         table.remove(mailbox.msg, i)
         numdeleted = numdeleted + 1
      end
   end

   if numdeleted > 0 then
      et.G_Print( string.format("%s: auto cleaned %d messages.\n", MODULENAME, numdeleted) )
   end
end
