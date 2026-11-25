import { Link } from "react-router";
import { useTranslation } from "react-i18next";
import { AppIcon } from "~/components/icons";

export function RadarWaiting({ message, connection = false }: { message: string; connection?: boolean }) {
  const { t } = useTranslation();
  return (
    <div className="radar-map-status" role="status">
      <span className="radar-waiting-icon"><AppIcon name="radar" size={25} /></span>
      <span>{t(message)}</span>
      {connection && <Link to="/">{t("Reader 连接")}</Link>}
    </div>
  );
}
