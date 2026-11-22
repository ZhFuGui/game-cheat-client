import {lazy} from "react";
import {AppIcon} from "~/components/icons";
import {PageScaffold} from "~/framework/pages/PageScaffold";
import {readerPages} from "~/navigation/pageGroups";
import {ReaderConnection} from "~/features/reader/ReaderConnection";
const views={connection:ReaderConnection,dashboard:lazy(()=>import("~/features/data-dashboard/DataDashboard"))};
const icons={connection:<AppIcon name="connection" size={14}/>,dashboard:<AppIcon name="dashboard" size={14}/>};
export default function Reader(){return <PageScaffold group={readerPages} views={views} icons={icons} className="reader-page"/>;}
