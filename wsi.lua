-- wsi.lua - weaponstat string interpreter for etmain
-- this module interprets the console printed weaponstat strings prefixed by 'WeaponStats:' and brings them into a more readable form
--
-- script by fart11eleven - nmueller1 at gmx dot net
--

WS_KNIFE = 0
WS_LUGER = 1
WS_COLT = 2
WS_MP40 = 3
WS_THOMPSON = 4
WS_STEN = 5
WS_FG42 = 6
WS_PANZERFAUST = 7
WS_FLAMETHROWER = 8
WS_GRENADE = 9
WS_MORTAR = 10
WS_DYNAMITE = 11
WS_AIRSTRIKE = 12
WS_ARTILLERY = 13
WS_SYRINGE = 14
WS_SMOKE = 15
WS_SATCHEL = 16
WS_GRENADELAUNCHER = 17
WS_LANDMINE = 18
WS_MG42 = 19
WS_GARAND = 20
WS_K43 = 21

SK_BATTLE_SENSE = 0
SK_EXPLOSIVES_AND_CONSTRUCTION = 1
SK_FIRST_AID = 2
SK_SIGNALS = 3
SK_LIGHT_WEAPONS = 4
SK_HEAVY_WEAPONS = 5
SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS = 6

-- callback - from here we catch the string and hand it out to our interpreter
function et_Print( text )
   local a, b  = string.find( text, "WeaponStats:" )
   if a ~= nil then
      local stat, weaponinfo, skillinfo = weaponstats_interpret( text )

      -- print what we received
      print_interpreted_result( text, stat, weaponinfo, skillinfo )
   end
end

-- takes a string and returns a set of substrings
function explode( div, str ) -- credit: http://richard.warburton.it
   if ( div=='' ) then return false end
   local pos, arr = 0, { }

   -- for each divider found
   for st, sp in function() return string.find( str, div, pos, true ) end do
      table.insert( arr,string.sub( str, pos, st-1) ) -- Attach chars left of current divider
      pos = sp + 1 -- Jump past current divider
   end
   table.insert( arr, string.sub( str, pos ) ) -- Attach chars right of last divider
   return arr
end

-- checks if flag is set
function check_flag( n, flag )
   return math.mod(n, 2*flag) >= flag
end

-- print interpreted result
function print_interpreted_result ( text, stat, weaponinfo, skillinfo )   

   et.G_Print("\n-- result of stat table\nclientnum "..stat.clientnum.."\nsess.rounds "..stat.rounds.."\ndamage_given "..stat.damage_given.."\ndamage_received "..stat.damage_received.."\nteam_damage "..stat.team_damage.."\n")

   et.G_Print("\n-- result of weaponinfo table\n")
   for i = WS_KNIFE, WS_K43 do
      et.G_Print("WeaponInfo["..i.."]: "..weaponinfo[i].atts.." "..weaponinfo[i].deaths.." "..weaponinfo[i].headshots.." "..weaponinfo[i].hits.." "..weaponinfo[i].kills.."\n")
   end

   et.G_Print("\n-- result of skillinfo table\n")
   for i = SK_BATTLE_SENSE, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS do
      et.G_Print("SkillInfo["..i.."]: "..skillinfo[i].."\n")
   end

   et.G_Print("\n-- string received\n"..text.."\n")
end

-- interprets the console printed weaponstat strings prefixed by 'WeaponStats:'
function weaponstats_interpret( text )

   -- result tables
   local stat, weaponinfo, skillinfo = { }, { }, { }

   -- extract elements
   local elements = explode( " ", text )

   -- extract clientNum
   stat.clientnum = tonumber(elements[2])

   -- extract sess.round
   stat.rounds = tonumber(elements[3])

   -- extract weaponInfo
   local weaponmask = tonumber(elements[4])
   
   local ptr = 5
   if weaponmask ~= 0 then

      for i = WS_KNIFE, WS_K43 do

         weaponinfo[i] = { }

         if ( check_flag( weaponmask, 2^i ) ) then
            weaponinfo[i].atts = tonumber(elements[ptr])
            weaponinfo[i].deaths = tonumber(elements[ptr+1])
            weaponinfo[i].headshots = tonumber(elements[ptr+2])
            weaponinfo[i].hits = tonumber(elements[ptr+3])
            weaponinfo[i].kills = tonumber(elements[ptr+4])
            ptr = ptr + 5
         else
            weaponinfo[i].atts = 0
            weaponinfo[i].deaths = 0
            weaponinfo[i].headshots = 0
            weaponinfo[i].hits = 0
            weaponinfo[i].kills = 0
         end
      end

      -- extract damage_given, damage_received and team_damage
      stat.damage_given = tonumber(elements[ptr])
      stat.damage_received = tonumber(elements[ptr+1])
      stat.team_damage = tonumber(elements[ptr+2])
      ptr = ptr + 3
   else
      for i = WS_KNIFE, WS_K43 do

         weaponinfo[i] = { }

         weaponinfo[i].atts = 0
         weaponinfo[i].deaths = 0
         weaponinfo[i].headshots = 0
         weaponinfo[i].hits = 0
         weaponinfo[i].kills = 0
      end

      stat.damage_given = 0
      stat.damage_received = 0
      stat.team_damage = 0
   end

   -- extract skillInfo
   local skillmask = tonumber(elements[ptr])
   ptr = ptr + 1

   if skillmask ~= 0 then

      for i = SK_BATTLE_SENSE, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS do

         if ( check_flag( skillmask, 2^i ) ) then
            skillinfo[i] = tonumber(elements[ptr])
            ptr = ptr + 1
         else
            skillinfo[i] = 0
         end
      end
   else
      for i = SK_BATTLE_SENSE, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS do
         skillinfo[i] = 0
      end
   end

   return stat, weaponinfo, skillinfo
end