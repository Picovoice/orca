import argparse

from tqdm import tqdm

from src import LLM, LLMs


SYSTEM_PROMPT = """
        You are a friendly voice assistant in customer service of an e-commerce platform.
        Use natural, conversational language that are clear and easy to follow (short sentences, simple words).
        Only use english letters and punctuation, no special characters.
        Be verbose.
        Keep the conversation flowing naturally.
        Don't use lists.
        If the customer was successful, say "Great!" and ask if they need help with anything else.
        """


def main(args: argparse.Namespace) -> None:

    sentences = []

    first_sentence = """
    Hi, I'm trying to place an order on your webpage but I'm having trouble with the checkout process. 
    Can you help me?"""
    second_sentence = "The place order button isn't working."

    llm = LLM.create(LLMs.OPENAI, access_key=args.openai_access_key, assistant_prompt=SYSTEM_PROMPT)

    for _ in tqdm(range(50)):
        llm_message = "".join([t for t in llm.chat(first_sentence) if t is not None])
        sentences.append(llm_message)

        #print(llm_message)

        llm_message = "".join([t for t in llm.chat(second_sentence) if t is not None])
        sentences.append(llm_message)

        #print(llm_message)

        # TODO: implement this method if using this script
        llm.reset_history()

    print("=============================================================================")

    # print sentences like a python list in the following format
    # [
    #     "sentence 1",
    #     "sentence 2",
    # ]

    print("[")
    for sentence in sentences:
        sentence = sentence.replace("\n", "")
        sentence = sentence.replace("\"Place Order\"", "Place Order")
        print(f'    "{sentence}",')
    print("]")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Text-to-speech streaming synthesis")

    parser.add_argument(
        "--llm",
        default=LLMs.DUMMY.value,
        choices=[l.value for l in LLMs],
        help="Choose LLM to use")
    parser.add_argument(
        "--openai-access-key",
        help="Open AI access key. Needed when using openai models")

    main(parser.parse_args())
