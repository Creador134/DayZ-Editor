class BlowoutEventSettings
{
	
	// Amount of waves
	int WaveCount = 10;
	
	// Time between waves in seconds
	float TimeBetweenWaves = 1;
	
	// Time between StartBlowout and actual event in seconds
	float BlowoutDelay = 30; // 120
	
	float ImpactShockDamage = 15;
	
	float BlowoutSize = 10000;
	float BlowoutCount = 3;
	
	static ref array<vector> GetAlarmPositions()
	{
		
		ref array<vector> alarm_positions = {};
		string world_name;
		GetGame().GetWorldName(world_name);
		string cfg = "CfgWorlds " + world_name + " Names";		
		
		string allowed_types = "Capital City";
		
		for (int i = 0; i < GetGame().ConfigGetChildrenCount(cfg); i++) {
			string city;
			GetGame().ConfigGetChildName(cfg, i, city);			
			vector city_position = GetGame().ConfigGetVector(string.Format("%1 %2 position", cfg, city));
			if (allowed_types.Contains(GetGame().ConfigGetTextOut(string.Format("%1 %2 type", cfg, city)))) {
				alarm_positions.Insert(city_position);
			}
		}
		
		return alarm_positions;
	}
}