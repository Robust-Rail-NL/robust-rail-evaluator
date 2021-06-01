import gym
import importlib
from gym import spaces
from rl.conv import TORSConverter
from manager.scenario_generator import ScenarioGeneratorFromFolder
from pyTORS import Engine, Action, ScenarioFailedError, InvalidActionError

class TORSEnv(gym.Env):
    metadata = {'render.modes': ['human']}

    def __init__(self, config):
        super(TORSEnv, self).__init__()
        print("Init Env")
        self.config = config
        self.engine = Engine(config['data folder']) 
        self.location = self.engine.get_location()
        self.number_of_trains = 2
        self.scenario_generator = self._get_generator(self.number_of_trains)
        self.scenario_generator.initialize(self.engine, config['scenario'])
        self.state = None
        self.scenario = None
        self.converter = self._get_converter()
        self.reset()
        self.action_space = self.converter.get_action_space(self.state)
        self.observation_space = self.converter.get_observation_space(self.state)

    def step(self, action):
        reward = 0
        done = False
        info = {}
        pos_actions = []
        if not isinstance(action, Action):
            try:
                action = self.converter.convert_action(self.state, action)
            except Exception as e:
                print("Exception in converting action (" + str(action) +  "): ", e)
                reward = 0
                done=True
        if not done:
            try:
                print("Applying action " +str(action))
                self.engine.apply_action_and_step(self.state, action)
                reward -= 0.005
                pos_actions = self.engine.get_valid_actions(self.state)
            except ScenarioFailedError:
                print("Scenario failed after action " + str(action))
                reward = 0
                done = True
            except InvalidActionError:
                print("Invalid action " + str(action))
                reward = 0
                done = True
        if not done:
            if len(pos_actions)==0:
                done = True
                reward = 0 
                if len(self.state.incoming_trains) == 0:
                    n_trains_left = 0
                    for out in self.state.outgoing_trains:
                        n_trains_left += out.shunting_unit.number_of_trains
                    reward = 1.0 - float(n_trains_left) / float(self.number_of_trains)
                print("End of scenario. Reward: " + str(reward))
        return self.converter.convert_state(self.state), reward, done, info
        
    def reset(self):
        print("Reset Env")
        self._reset()
        self.engine.step(self.state)
        return self.converter.convert_state(self.state)
        
    def render(self, mode='human'):
        pass

    def close (self):
        print("Close Env")
        if not self.state is None:
            self.engine.end_session(self.state)


    def _reset(self):
        if not self.state is None:
            self.engine.end_session(self.state)
            self.state = None
        del self.scenario

        # TODO: Shunting units are stored in incoming and outgoin goals, which are deleted when the scenario is deleted,
        # and also in the state, which is deleted when the state is deleted.

        self.scenario = self.scenario_generator.generate_scenario()
        self.state = self.engine.start_session(self.scenario)
        self.result = 0

    def _get_generator(self, n_trains):
        generator_str = self.config.generator['class']
        generator_lst = generator_str.split('.')
        _module = importlib.import_module(".".join(generator_lst[:-1]))
        _class = getattr(_module, generator_lst[-1])
        config = self.config.generator.copy()
        del config['class']
        if generator_str in self.config:
            config.update(self.config[generator_str])
        return ScenarioGeneratorFromFolder(_class, n_trains=n_trains, **config)

    def _get_converter(self):
        converter_str = self.config.converter['class']
        converter_lst = converter_str.split('.')
        _module = importlib.import_module(".".join(converter_lst[:-1]))
        _class = getattr(_module, converter_lst[-1])
        config = self.config.converter.copy()
        del config['class']
        if converter_str in self.config:
            config.update(self.config[converter_str])
        return _class(self.engine, self.location, **config)
	
    def print(self, text):
    	print(text)

    def write_to_file(self, filename):
        print("Get Result")
        result = self.engine.get_result(self.state)
        print("Serialize result")
        result.serialize_to_file(self.engine, filename)
