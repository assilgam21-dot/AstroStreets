return {
    name = "Fire Rune",
    width = 5,
    height = 3,
    damagePerSecond = 1,

    light = { r = 1.0, g = 0.55, b = 0.18, radius = 6.0, intensity = 1.7 },

    frames = {
        { sprite = Sprite.FireRune0, dur = 0.18 },
        { sprite = Sprite.FireRune1, dur = 0.16 },
        { sprite = Sprite.FireRune2, dur = 0.20 },
    },

    onUpdate = function(self, dt)
        self.tick = (self.tick or 0) + dt
        if self.tick < 1.0 then return end
        self.tick = self.tick - 1.0
        for _, u in ipairs(self.unitsInside()) do
            u:takeDamage(self.damagePerSecond, DamageSchool.Fire)
        end
    end,

    onEnter = function(self, unit)
        self.log(unit:name() .. " steps into the fire rune!")
    end,

    onExit = function(self, unit)
        self.log(unit:name() .. " leaves the fire rune.")
    end,
}
