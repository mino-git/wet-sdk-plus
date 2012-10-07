
function et_InitGame( levelTime, randomSeed, restart )
   et.G_Print("Lua Event: et_InitGame\n")
end


function et_ShutdownGame( restart )
   et.G_Print("Lua Event: et_ShutdownGame\n")
end


--function et_RunFrame( levelTime )
--   et.G_Print("Lua Event: et_RunFrame\n")
--end


function et_Quit()
   et.G_Print("Lua Event: et_Quit()\n")
end


function et_ClientConnect( clientNum, firstTime, isBot )
   et.G_Print("Lua Event: et_ClientConnect\n")
   return nil
end


function et_ClientDisconnect( clientNum )
   et.G_Print("Lua Event: et_ClientDisconnect\n")
end


function et_ClientBegin( clientNum )
   et.G_Print("Lua Event: et_ClientBegin\n")
end


function et_ClientUserinfoChanged( clientNum )
   et.G_Print("Lua Event: et_ClientUserinfoChanged\n")
end


function et_ClientSpawn( clientNum, revived )
   et.G_Print("Lua Event: et_ClientSpawn\n")
end


function et_ClientCommand( clientNum, command )
   et.G_Print("Lua Event: et_ClientCommand\n")
   return 0
end


function et_ConsoleCommand()
   et.G_Print("Lua Event: et_ConsoleCommand\n")
   return 0
end


function et_UpgradeSkill( cno, skill )
   et.G_Print("Lua Event: et_UpgradeSkill\n")
   return 0
end


function et_SetPlayerSkill( cno, skill )
   et.G_Print("Lua Event: et_SetPlayerSkill\n")
   return 0
end


--function et_Print( text )
--   et.G_Print("Lua Event: et_Print\n")
--end


function et_Obituary( victim, killer, meansOfDeath )
   et.G_Print("Lua Event: et_Obituary\n")
end
