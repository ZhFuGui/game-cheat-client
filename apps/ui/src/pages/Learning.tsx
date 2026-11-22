import { useLearningSession } from "~/features/learning/useLearningSession";
import { LearningCapture } from "~/features/learning/LearningCapture";
import { LearningModel } from "~/features/learning/LearningModel";
import { LearningConfirmation } from "~/features/learning/LearningConfirmation";

export default function Learning({
  embedded = false,
}: { embedded?: boolean } = {}) {
  const session = useLearningSession();
  return (
    <div className={`${embedded ? "embedded-learning" : "page"} learning-page`}>
      <LearningCapture session={session} />
      <LearningModel session={session} />
      <LearningConfirmation session={session} />
    </div>
  );
}
