import { useTranslation } from "react-i18next";
import type { Cs2RadarPlayer } from "@mousebox/shared";

export function RadarRoster({ team, players }: { team: 2 | 3; players: readonly Cs2RadarPlayer[] }) {
  const { t } = useTranslation();
  const name = team === 3 ? "CT" : "T";
  const members = players.filter(player => player.team === team);
  const alive = members.some(player => player.alive === null) ? "—" : members.filter(player => player.alive).length;
  return <aside className={`radar-roster radar-roster-${name.toLowerCase()}`} aria-label={t("{{team}} 队伍", { team: name })}>
    <div className="radar-roster-header">
      <strong><i aria-hidden="true" />{name}</strong>
      {members.length > 0 && <span title={t("存活 / 总人数")}>{alive} / {members.length}</span>}
    </div>
    <ul>
      {members.map(player => {
        const name = player.name || t("未命名玩家");
        const gun = player.weapon?.name.replace(/^weapon_/, "").replaceAll("_", " ");
        const ammo = player.weapon?.clip;
        const reserve = player.weapon?.reserve;
        return <li key={player.controller} data-alive={player.alive === false ? "false" : undefined}>
          <div className="radar-player-title">
            <strong title={name}>{name}</strong>
            {player.local && <span>{t("本地")}</span>}
            {player.alive === false && <span>{t("已阵亡")}</span>}
          </div>
          <div className="radar-player-stats">
            <span>{player.health ?? "—"} HP</span>
            {player.armor !== null && <span>{t("护甲 {{value}}", { value: player.armor })}</span>}
          </div>
          {player.health !== null && player.maxHealth !== null && player.maxHealth > 0 &&
            <span className="radar-player-health" aria-hidden="true">
              <span style={{ width: `${Math.max(0, Math.min(100, player.health * 100 / player.maxHealth))}%` }} />
            </span>}
          {gun && <div className="radar-player-weapon">
            <span title={gun}>{gun}</span>
            {ammo != null && ammo >= 0 && <span>{ammo}{reserve != null && reserve >= 0 ? ` / ${reserve}` : ""}</span>}
          </div>}
        </li>;
      })}
    </ul>
  </aside>;
}
