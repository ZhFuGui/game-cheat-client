import {AppIcon} from '~/components/icons'
import {PageScaffold} from '~/framework/pages/PageScaffold'
import {mousePages} from '~/navigation/pageGroups'
import Learning from './Learning'
import {MouseCharacteristics} from '~/features/mouse-debug/MouseCharacteristics'

function LearningView(){return <Learning embedded/>}
const views={learning:LearningView,characteristics:MouseCharacteristics}
const icons={speedLearning:<AppIcon name="speedLearning" size={14}/>,mouseDevice:<AppIcon name="mouseDevice" size={14}/>}
export default function MouseDebug(){return <PageScaffold group={mousePages} views={views} icons={icons} className="mouse-debug-page"/>}
